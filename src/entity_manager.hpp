//
//  entity_manager.hpp
//  nice
//
//  Created by George Watson on 29/08/2025.
//

#pragma once

#include "nice.hpp"
#include "global.hpp"
#include "components.hpp"
#include "vertex_batch.hpp"
#include "asset_manager.hpp"
#include "job_queue.hpp"
#include "camera.hpp"
#include "basic.glsl.h"
#include "fmt/format.h"
#include <iostream>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
#include <atomic>

struct BasicVertex {
    glm::vec2 position;
    glm::vec2 texcoord;
};

struct PathRequest {
    flecs::entity entity;
    Chunk *chunk;
    glm::vec2 start;
    glm::vec2 end;
};

enum PathRequestResult {
    StillSearching,
    TargetUnreachable,
    TargetReached
};

#define $Entities EntityManager::instance()

class EntityManager: public Global<EntityManager> {
    using EntityMap = std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::vector<flecs::entity>>>;
    using BatchMap = std::unordered_map<uint32_t, std::unordered_map<uint32_t, VertexBatch<BasicVertex>>>;
    using EntityMapCache = std::unordered_map<flecs::entity, std::pair<uint32_t, uint32_t>>;

    struct DrawCall {
        VertexBatch<BasicVertex>* batch;
        std::vector<flecs::entity> entities;
        Texture* texture; // Pre-resolved texture to avoid lock in worker thread
    };

    flecs::world *_world = nullptr;
    EntityMap _entities;
    mutable std::shared_mutex _entities_lock;
    EntityMapCache _entity_cache;
    BatchMap batches;
    mutable std::mutex _batches_lock;
    JobQueue<DrawCall> _build_queue;
    std::atomic<size_t> _pending_jobs{0};
    std::mutex _completion_mutex;
    std::condition_variable _completion_cv;
    
    // Texture registry
    std::unordered_map<uint32_t, Texture*> _textures;
    std::unordered_map<std::string, uint32_t> _texture_paths;
    std::atomic<uint32_t> _next_texture_id{1}; // Start from 1, 0 can be reserved for "no texture"
    mutable std::mutex _texture_lock;

    JobQueue<PathRequest> _path_request_queue;
    std::unordered_map<flecs::entity, std::pair<PathRequestResult, std::vector<glm::vec2>>> _waypoints;
    mutable std::mutex _waypoints_mutex;
    ThreadSafeSet<flecs::entity> _entities_requesting_paths;

    BasicVertex* generate_quad(LuaEntity *entity_data, Texture* texture) {
        int _texture_width = texture->width();
        int _texture_height = texture->height();
        int width = entity_data->width > 0 ? entity_data->width : _texture_width;
        int height = entity_data->height > 0 ? entity_data->height : _texture_height;
        float scaled_width = width * entity_data->scale_x;
        float scaled_height = height * entity_data->scale_y;
        float center_x = entity_data->x + scaled_width * .5f;
        float center_y = entity_data->y + scaled_height * .5f;
        glm::vec2 corners[4] = {
            {-scaled_width * .5f, -scaled_height * .5f}, // Top-left
            { scaled_width * .5f, -scaled_height * .5f}, // Top-right
            { scaled_width * .5f,  scaled_height * .5f}, // Bottom-right
            {-scaled_width * .5f,  scaled_height * .5f}, // Bottom-left
        };
        
        glm::vec2 _positions[4];
        if (entity_data->rotation != .0f) {
            float cos_rot = std::cos(entity_data->rotation);
            float sin_rot = std::sin(entity_data->rotation);
            for (int i = 0; i < 4; i++) {
                float rotated_x = corners[i].x * cos_rot - corners[i].y * sin_rot;
                float rotated_y = corners[i].x * sin_rot + corners[i].y * cos_rot;
                _positions[i] = {center_x + rotated_x, center_y + rotated_y};
            }
        } else
            for (int i = 0; i < 4; i++)
                _positions[i] = {center_x + corners[i].x, center_y + corners[i].y};

        float iw = 1.f / _texture_width;
        float ih = 1.f / _texture_height;
        float clip_width = entity_data->clip_width ? entity_data->clip_width : _texture_width;
        float clip_height = entity_data->clip_height ? entity_data->clip_height : _texture_height;
        float tl = entity_data->clip_x * iw;
        float tt = entity_data->clip_y * ih;
        float tr = (entity_data->clip_x + clip_width) * iw;
        float tb = (entity_data->clip_y + clip_height) * ih;
        glm::vec2 _texcoords[4] = {
            {tl, tt}, // top left
            {tr, tt}, // top right
            {tr, tb}, // bottom right
            {tl, tb}, // bottom left
        };

        BasicVertex *v = new BasicVertex[6];
        static uint16_t _indices[] = {0, 1, 2, 2, 3, 0};
        for (int i = 0; i < 6; i++) {
            v[i].position = _positions[_indices[i]];
            v[i].texcoord = _texcoords[_indices[i]];
        }
        return v;
    }

    void wait_for_jobs_completion() {
        std::unique_lock<std::mutex> lock(_completion_mutex);
        _completion_cv.wait(lock, [this] { return _pending_jobs.load() == 0; });
    }

public:
    EntityManager()
    : _build_queue([this](DrawCall call) {
        // Ensure we always decrement the pending counter and notify, even if processing throws
        try {
            for (const auto& entity : call.entities) {
                if (!entity.is_alive())
                    continue;
                LuaEntity *entity_data = entity.get_mut<LuaEntity>();
                if (!call.texture) // Safety check
                    continue;
                BasicVertex *vertices = generate_quad(entity_data, call.texture);
                call.batch->add_vertices(vertices, 6);
                delete[] vertices;
            }
            call.batch->build();
        } catch (...) {
            // Swallow exceptions to avoid terminating the worker thread; still ensure counter is decremented.
        }

        // Decrement pending jobs counter and notify
        _pending_jobs.fetch_sub(1);
        _completion_cv.notify_all();
    })
    , _path_request_queue([&](PathRequest request) {
        auto path = request.chunk->astar(request.start, request.end);
        std::lock_guard<std::mutex> lock(_waypoints_mutex);
        if (!path.has_value() || path->empty()) {
            _waypoints[request.entity].first = PathRequestResult::TargetUnreachable;
            _entities_requesting_paths.erase(request.entity);
        } else {
            _waypoints[request.entity].first = PathRequestResult::StillSearching;
            _waypoints[request.entity].second.insert(_waypoints[request.entity].second.end(), path->begin(), path->end());
            if (path->end()->x == request.end.x &&
                path->end()->y == request.end.y) {
                _waypoints[request.entity].first = PathRequestResult::TargetReached;
                _entities_requesting_paths.erase(request.entity);
            } else
                _path_request_queue.enqueue({request.entity, request.chunk, path->back(), request.end});
        }
    }) {}

    void add_entity_target(flecs::entity entity, LuaTarget target) {
        std::lock_guard<std::mutex> lock(_waypoints_mutex);
        _waypoints[entity] = {PathRequestResult::StillSearching, {}};
        _entities_requesting_paths.insert(entity);
    }

    std::optional<std::pair<PathRequestResult, glm::vec2>> get_next_waypoint(flecs::entity entity) {
        std::lock_guard<std::mutex> lock(_waypoints_mutex);
        auto it = _waypoints.find(entity);
        if (it == _waypoints.end() || it->second.second.empty())
            return std::nullopt;
        glm::vec2 next = it->second.second.front();
        it->second.second.erase(it->second.second.begin());
        if (it->second.first == PathRequestResult::TargetReached || it->second.first == PathRequestResult::TargetUnreachable)
            _waypoints.erase(it);
        return {{it->second.first, next}};
    }

    void clear_target(flecs::entity entity) {
        std::lock_guard<std::mutex> lock(_waypoints_mutex);
        _waypoints.erase(entity);
        _entities_requesting_paths.erase(entity);
        if (entity.has<LuaTarget>())
            entity.remove<LuaTarget>();
    }

    void set_world(flecs::world *world) {
        _world = world;
    }

    Texture* get_texture_by_id(uint32_t texture_id) {
        std::lock_guard<std::mutex> lock(_texture_lock);
        auto it = _textures.find(texture_id);
        return (it != _textures.end()) ? it->second : nullptr;
    }

    uint32_t register_texture(const char *path) {
        std::lock_guard<std::mutex> lock(_texture_lock);
        // Check if texture is already registered
        auto path_it = _texture_paths.find(path);
        if (path_it != _texture_paths.end())
            return path_it->second; // Return existing texture ID
        // Load the texture using the asset manager
        Texture* texture = $Assets.get<Texture>(path);
        if (!texture || !texture->is_valid())
            return 0; // Return 0 for invalid texture
        // Assign new texture ID and register it
        uint32_t texture_id = _next_texture_id.fetch_add(1);
        _textures[texture_id] = texture;
        _texture_paths[path] = texture_id;
        return texture_id;
    }

    void add_entity(flecs::entity entity) {
        std::lock_guard<std::shared_mutex> lock(_entities_lock);
        LuaEntity *entity_data = entity.get_mut<LuaEntity>();
        _entities[entity_data->z_index][entity_data->texture_id].push_back(entity);
        _entity_cache[entity] = {entity_data->z_index, entity_data->texture_id};
    }

    void remove_entity(flecs::entity entity, bool lock=true) {
        std::shared_lock<std::shared_mutex> unlock;
        if (lock)
            unlock = std::shared_lock<std::shared_mutex>(_entities_lock);
        LuaEntity *entity_data = entity.get_mut<LuaEntity>();
        _entity_cache.erase(entity);
        auto &layer = _entities[entity_data->z_index];
        auto &vec = layer[entity_data->texture_id];
        vec.erase(std::remove(vec.begin(), vec.end(), entity), vec.end());
        if (vec.empty())
            layer.erase(entity_data->texture_id);
        if (layer.empty())
            _entities.erase(entity_data->z_index);
        if (lock)
            unlock.unlock();
    }

    void update_entity(flecs::entity entity, LuaEntity &entity_data, LuaChunk &chunk, bool lock=true) {
        if (!entity.is_alive())
            return;
        std::shared_lock<std::shared_mutex> unlock;
        if (lock)
            unlock = std::shared_lock<std::shared_mutex>(_entities_lock);
        auto it = _entity_cache.find(entity);
        if (it == _entity_cache.end()) {
            entity.destruct();
            if (lock)
                unlock.unlock();
        }

        glm::vec2 world = Camera::tile_to_world(chunk.x, chunk.y, entity_data.x, entity_data.y);
        entity_data.x = world.x;
        entity_data.y = world.y;

        auto [old_z_index, old_texture_id] = it->second;
        if (old_z_index == entity_data.z_index && old_texture_id == entity_data.texture_id) {
            if (lock)
                unlock.unlock();
            return;
        }
        // Remove from old location
        auto &old_layer = _entities[old_z_index];
        auto &old_vec = old_layer[old_texture_id];
        old_vec.erase(std::remove(old_vec.begin(), old_vec.end(), entity), old_vec.end());
        if (old_vec.empty())
            old_layer.erase(old_texture_id);
        if (old_layer.empty())
            _entities.erase(old_z_index);
        // Add to new location
        _entities[entity_data.z_index][entity_data.texture_id].push_back(entity);
        // Update cache
        it->second = {entity_data.z_index, entity_data.texture_id};

        if (lock)
            unlock.unlock();
    }

    static Rect entity_bounds(const LuaEntity& entity_data) {
        return Rect(static_cast<int>(entity_data.x),
                    static_cast<int>(entity_data.y),
                    static_cast<int>(entity_data.width * entity_data.scale_x),
                    static_cast<int>(entity_data.height * entity_data.scale_y));
    }

    void finalize(Camera *camera) {
        Rect camera_bounds = camera->bounds();
        EntityMap map_copy;
        
        // Copy entities while holding lock, then release it quickly
        {
            std::shared_lock<std::shared_mutex> lock(_entities_lock);
            map_copy = _entities;
            
            // Build a filtered snapshot into map_copy while holding the lock
            // (we'll release the lock and operate on the copy afterwards).
            for (auto it = _entities.begin(); it != _entities.end(); ++it) {
                auto &layer = it->second;
                auto &copy_layer = map_copy[it->first];
                for (auto layer_it = layer.begin(); layer_it != layer.end(); ++layer_it) {
                    auto &vec = layer_it->second;
                    auto &copy_vec = copy_layer[layer_it->first];
                    // Copy only alive entities that intersect the camera bounds
                    for (auto &entity : vec) {
                        if (!entity.is_alive())
                            continue;
                        LuaEntity *entity_data = entity.get_mut<LuaEntity>();
                        Rect bounds = entity_bounds(*entity_data);
                        if (bounds.intersects(camera_bounds))
                            copy_vec.push_back(entity);
                    }
                }
                // Sort entities in layer by y-axis
                for (auto &pair : copy_layer)
                    std::sort(pair.second.begin(), pair.second.end(), [](flecs::entity a, flecs::entity b) {
                        LuaEntity *ra = a.get_mut<LuaEntity>();
                        LuaEntity *rb = b.get_mut<LuaEntity>();
                        return ra->y < rb->y;
                    });
            }
        }
        
        // Pre-fetch textures to avoid holding texture lock during batch operations
        std::unordered_map<uint32_t, Texture*> texture_cache;
        {
            std::lock_guard<std::mutex> texture_lock(_texture_lock);
            for (auto [layer, v] : map_copy)
                for (auto [texture_id, vv] : v) {
                    auto it = _textures.find(texture_id);
                    if (it != _textures.end())
                        texture_cache[texture_id] = it->second;
                }
        }
        
        // Now acquire batch lock and set up batches
        std::lock_guard<std::mutex> batch_lock(_batches_lock);
        if (!batches.empty())
            throw std::runtime_error("Batches were not flushed before finalize");
        batches.reserve(map_copy.size());
        for (auto [layer, v]: map_copy) {
            batches[layer].reserve(v.size());
            for (auto [texture_id, vv]: v) {
                auto texture_it = texture_cache.find(texture_id);
                if (texture_it == texture_cache.end())
                    continue;
                Texture* texture = texture_it->second;
                VertexBatch<BasicVertex> *batch = &batches[layer][texture_id];
                batch->set_texture(texture);
                _pending_jobs.fetch_add(1);
                _build_queue.enqueue(DrawCall{batch, std::move(vv), texture});
            }
        }
    }

    void flush(Camera *camera) {
        // Wait for all build jobs to complete before flushing
        wait_for_jobs_completion();
        {
            std::lock_guard<std::mutex> batch_lock(_batches_lock);
            for (auto &[layer, layer_batches] : batches)
                for (auto &[texture_id, batch] : layer_batches) {
                    if (batch.empty() || !batch.is_ready())
                        continue;
                    vs_params_t vs_params = { .mvp = camera->matrix() };
                    sg_range params = SG_RANGE(vs_params);
                    sg_apply_uniforms(UB_vs_params, &params);
                    batch.flush();
                }
        }
        batches.clear();
    }

    void clear() {
        // Wait for any pending jobs to complete before clearing
        wait_for_jobs_completion();
        
        // Acquire locks in consistent order to avoid deadlocks: texture -> entities -> batches
        std::lock_guard<std::mutex> texture_lock(_texture_lock);
        std::lock_guard<std::shared_mutex> entities_lock(_entities_lock);
        std::lock_guard<std::mutex> batch_lock(_batches_lock);
        
        _textures.clear();
        _texture_paths.clear();
        _entities.clear();
        batches.clear();
        _next_texture_id.store(1);
    }

    std::shared_mutex& entities_lock() {
        return _entities_lock;
    }
};
