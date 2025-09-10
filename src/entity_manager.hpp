//
//  entity_manager.hpp
//  nice
//
//  Created by George Watson on 29/08/2025.
//

#pragma once

#include "global.hpp"
#include "vertex_batch.hpp"
#include "asset_manager.hpp"
#include "job_queue.hpp"
#include "camera.hpp"
#include "basic.glsl.h"
#include "chunk_manager.hpp"
#include "fmt/format.h"
#include <iostream>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
#include <atomic>

ECS_STRUCT(LuaChunkEntity, {
    float x;
    float y;
    float width;
    float height;
    uint32_t texture_id;
    uint32_t z_index;
    float rotation;
    float scale_x;
    float scale_y;
    uint32_t clip_x;
    uint32_t clip_y;
    uint32_t clip_width;
    uint32_t clip_height;
    float speed;
});

struct LuaChunk {
    uint32_t x;
    uint32_t y;
};

struct LuaTarget {
    int x;
    int y;
};

struct LuaWaypoint {
    int x;
    int y;
};

struct BasicVertex {
    glm::vec2 position;
    glm::vec2 texcoord;
};

struct PathRequest {
    flecs::entity entity;
    LuaChunk chunk;
    glm::vec2 start;
    glm::vec2 end;
};

enum PathRequestResult {
    StillSearching,
    TargetUnreachable,
    TargetReached
};

#define $Entities EntityManager::instance()

template<typename EntityType>
class EntityFactory {
    using EntityMap = std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::vector<flecs::entity>>>;
    using BatchMap = std::unordered_map<uint32_t, std::unordered_map<uint32_t, VertexBatch<BasicVertex>>>;
    using EntityMapCache = std::unordered_map<flecs::entity, std::pair<uint32_t, uint32_t>>;

    struct DrawCall {
        VertexBatch<BasicVertex>* batch;
        std::vector<flecs::entity> entities;
        Texture* texture; // Pre-resolved texture to avoid lock in worker thread
    };
    
    EntityMap _entities;
    mutable std::shared_mutex _entities_lock;
    EntityMapCache _entity_cache;
    BatchMap batches;
    mutable std::mutex _batches_lock;
    JobQueue<DrawCall> _build_queue;
    std::atomic<size_t> _pending_jobs{0};
    std::mutex _completion_mutex;
    std::condition_variable _completion_cv;

    BasicVertex* generate_quad(EntityType *entity_data, Texture* texture) {
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
    EntityFactory()
    : _build_queue([this](DrawCall call) {
        // Ensure we always decrement the pending counter and notify, even if processing throws
        try {
            for (const auto& entity : call.entities) {
                if (!entity.is_alive())
                    continue;
                EntityType *entity_data = entity.template get_mut<EntityType>();
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
    }) {};

    void add_entity(flecs::entity entity) {
        std::lock_guard<std::shared_mutex> lock(_entities_lock);
        EntityType *entity_data = entity.template get_mut<EntityType>();
        _entities[entity_data->z_index][entity_data->texture_id].push_back(entity);
        _entity_cache[entity] = {entity_data->z_index, entity_data->texture_id};
    }

    void remove_entity(flecs::entity entity, bool lock=true) {
        std::shared_lock<std::shared_mutex> unlock;
        if (lock)
            unlock = std::shared_lock<std::shared_mutex>(_entities_lock);
        EntityType *entity_data = entity.template get_mut<EntityType>();
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

    void update_entity(flecs::entity entity, EntityType &entity_data, LuaChunk &chunk, bool lock=true) {
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

    static Rect entity_bounds(const EntityType& entity_data) {
        return Rect(static_cast<int>(entity_data.x),
                    static_cast<int>(entity_data.y),
                    static_cast<int>(entity_data.width * entity_data.scale_x),
                    static_cast<int>(entity_data.height * entity_data.scale_y));
    }

    void finalize(Camera *camera); // Implementation moved after EntityManager definition

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
        
        std::lock_guard<std::shared_mutex> entities_lock(_entities_lock);
        std::lock_guard<std::mutex> batch_lock(_batches_lock);
        
        _entities.clear();
        _entity_cache.clear();
        batches.clear();
    }

    std::shared_mutex& entities_lock() {
        return _entities_lock;
    }
};

class ChunkEntityFactory: public EntityFactory<LuaChunkEntity> {
    JobQueue<PathRequest> _path_request_queue;
    std::unordered_map<flecs::entity, std::pair<PathRequestResult, std::vector<glm::vec2>>> _waypoints;
    mutable std::mutex _waypoints_mutex;
    ThreadSafeSet<flecs::entity> _entities_requesting_paths;

public:
    ChunkEntityFactory()
    : EntityFactory<LuaChunkEntity>()
    , _path_request_queue([&](PathRequest request) {
        std::cout << fmt::format("Processing path request for entity {}\n", request.entity.id());
        std::optional<std::vector<glm::vec2>> path = std::nullopt;
        
        $Chunks.get_chunk(request.chunk.x, request.chunk.y, [&](Chunk *c) {
            if (!c)
                return;
            // Convert world coordinates to tile coordinates within the chunk
            glm::vec2 start_world = request.start;
            glm::vec2 end_world = request.end;
            // Convert to chunk-local tile coordinates
            glm::vec2 chunk_world_pos = Camera::chunk_to_world(request.chunk.x, request.chunk.y);
            glm::vec2 start_tile = Camera::world_to_tile(start_world);
            glm::vec2 end_tile = Camera::world_to_tile(end_world);
            // Get tile coordinates relative to chunk
            int start_x = static_cast<int>(start_tile.x) % CHUNK_WIDTH;
            int start_y = static_cast<int>(start_tile.y) % CHUNK_HEIGHT;
            int end_x = static_cast<int>(end_tile.x) % CHUNK_WIDTH;
            int end_y = static_cast<int>(end_tile.y) % CHUNK_HEIGHT;
            // Ensure coordinates are within chunk bounds
            start_x = glm::clamp(start_x, 0, CHUNK_WIDTH - 1);
            start_y = glm::clamp(start_y, 0, CHUNK_HEIGHT - 1);
            end_x = glm::clamp(end_x, 0, CHUNK_WIDTH - 1);
            end_y = glm::clamp(end_y, 0, CHUNK_HEIGHT - 1);
            path = c->astar(glm::vec2(start_x, start_y), glm::vec2(end_x, end_y));
            // Convert path back to world coordinates
            if (path.has_value())
                for (auto& point : path.value()) {
                    glm::vec2 world_pos = Camera::tile_to_world(request.chunk.x, request.chunk.y, 
                                                                static_cast<int>(point.x), static_cast<int>(point.y));
                    point = world_pos;
                }
        });
        
        std::lock_guard<std::mutex> lock(_waypoints_mutex);
        if (!path.has_value() || path->empty()) {
            std::cout << fmt::format("Pathfinding failed for entity {}\n", request.entity.id());
            _waypoints[request.entity].first = PathRequestResult::TargetUnreachable;
            _entities_requesting_paths.erase(request.entity);
        } else {
            std::cout << fmt::format("Pathfinding succeeded for entity {}, found path with {} points\n", 
                                   request.entity.id(), path->size());
            _waypoints[request.entity].first = PathRequestResult::StillSearching;
            
            // Store up to 3 waypoints ahead for caching
            int max_cache = std::min(3, static_cast<int>(path->size()));
            auto& waypoint_cache = _waypoints[request.entity].second;
            // Don't clear the cache - append new waypoints to existing ones
            
            for (int i = 1; i < max_cache + 1 && i < path->size(); ++i) { // Skip first point (current position)
                waypoint_cache.push_back(path->at(i));
                std::cout << fmt::format("Cached waypoint {}: ({}, {})\n", i, path->at(i).x, path->at(i).y);
            }
            
            // Check if the cached waypoints include the final target or if we're close enough
            bool target_reached = false;
            if (!waypoint_cache.empty()) {
                glm::vec2 last_cached = waypoint_cache.back();
                target_reached = (glm::distance(last_cached, request.end) < 1.0f) || 
                                (path->size() <= max_cache + 1);  // +1 because we skip the first point
            }
            
            if (target_reached) {
                _waypoints[request.entity].first = PathRequestResult::TargetReached;
                _entities_requesting_paths.erase(request.entity);
                std::cout << fmt::format("Pathfinding completed for entity {}, target reached\n", request.entity.id());
            } else {
                // Continue pathfinding from the last cached waypoint
                glm::vec2 continuation_start = path->at(max_cache);
                glm::vec2 target_chunk_pos = Camera::world_to_chunk(request.end);
                LuaChunk target_chunk = {static_cast<uint32_t>(target_chunk_pos.x), static_cast<uint32_t>(target_chunk_pos.y)};
                
                _path_request_queue.enqueue({request.entity, target_chunk, continuation_start, request.end});
                std::cout << fmt::format("Pathfinding partial for entity {}, continuing search from ({}, {})\n", 
                                       request.entity.id(), continuation_start.x, continuation_start.y);
            }
        }
    }) {}

    void add_entity_target(flecs::entity entity, LuaChunk chunk, LuaTarget target) {
        std::lock_guard<std::mutex> lock(_waypoints_mutex);
        _waypoints[entity] = {PathRequestResult::StillSearching, {}};
        _entities_requesting_paths.insert(entity);
        
        // Get entity's current position
        LuaChunkEntity *entity_data = entity.get_mut<LuaChunkEntity>();
        if (!entity_data) {
            std::cerr << "Warning: ChunkEntity missing LuaChunkEntity component in add_entity_target\n";
            return;
        }
        
        // Convert target tile coordinates to world coordinates
        glm::vec2 target_world = Camera::tile_to_world(chunk.x, chunk.y, target.x, target.y);
        
        // Current position is already in world coordinates
        glm::vec2 start_world = {entity_data->x, entity_data->y};
        
        std::cout << fmt::format("Setting target for entity {} from ({}, {}) to ({}, {}) in world coords\n", 
                                entity.id(), start_world.x, start_world.y, target_world.x, target_world.y);
        
        // Enqueue the path request with proper world coordinates
        _path_request_queue.enqueue({entity, chunk, start_world, target_world});
    }

    std::optional<std::pair<PathRequestResult, glm::vec2>> get_next_waypoint(flecs::entity entity) {
        std::lock_guard<std::mutex> lock(_waypoints_mutex);
        auto it = _waypoints.find(entity);
        if (it == _waypoints.end() || it->second.second.empty())
            return std::nullopt;
        
        glm::vec2 next = it->second.second.front();
        it->second.second.erase(it->second.second.begin());
        
        PathRequestResult status = it->second.first;
        
        // If this is the last waypoint and pathfinding is complete, return TargetReached
        if (it->second.second.empty() && status == PathRequestResult::TargetReached) {
            _waypoints.erase(it);
            return {{PathRequestResult::TargetReached, next}};
        }
        
        // If this is the last waypoint but pathfinding is still in progress, keep searching
        if (it->second.second.empty() && status == PathRequestResult::StillSearching)
            return {{PathRequestResult::StillSearching, next}};
        
        // Clean up if target is unreachable
        if (it->second.second.empty() && status == PathRequestResult::TargetUnreachable) {
            _waypoints.erase(it);
            return {{PathRequestResult::TargetUnreachable, next}};
        }
        
        // More waypoints available, keep searching
        return {{PathRequestResult::StillSearching, next}};
    }

    void clear_target(flecs::entity entity) {
        std::lock_guard<std::mutex> lock(_waypoints_mutex);
        _waypoints.erase(entity);
        _entities_requesting_paths.erase(entity);
    }
};

class EntityManager: public Global<EntityManager> {
    flecs::world *_world = nullptr;
    ChunkEntityFactory _chunk_entities;

    // Texture registry
    std::unordered_map<uint32_t, Texture*> _textures;
    std::unordered_map<std::string, uint32_t> _texture_paths;
    std::atomic<uint32_t> _next_texture_id{1}; // Start from 1, 0 can be reserved for "no texture"
    mutable std::mutex _texture_lock;

public:
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

    // Delegate entity operations to the chunk entity factory
    void add_chunk_entity(flecs::entity entity) {
        _chunk_entities.add_entity(entity);
    }

    void remove_chunk_entity(flecs::entity entity, bool lock = true) {
        _chunk_entities.remove_entity(entity, lock);
    }

    void update_chunk_entity(flecs::entity entity, LuaChunkEntity &entity_data, LuaChunk &chunk, bool lock = true) {
        _chunk_entities.update_entity(entity, entity_data, chunk, lock);
    }

    void add_entity_target(flecs::entity entity, LuaChunk chunk, LuaTarget target) {
        _chunk_entities.add_entity_target(entity, chunk, target);
    }

    std::optional<std::pair<PathRequestResult, glm::vec2>> get_next_waypoint(flecs::entity entity) {
        return _chunk_entities.get_next_waypoint(entity);
    }

    void clear_target(flecs::entity entity) {
        _chunk_entities.clear_target(entity);
    }

    void finalize(Camera *camera) {
        _chunk_entities.finalize(camera);
    }

    void flush(Camera *camera) {
        _chunk_entities.flush(camera);
    }

    void clear() {
        _chunk_entities.clear();
        std::lock_guard<std::mutex> lock(_texture_lock);
        _textures.clear();
        _texture_paths.clear();
        _next_texture_id.store(1);
    }

    std::shared_mutex& chunk_entities_lock() {
        return _chunk_entities.entities_lock();
    }

    static Rect entity_bounds(const LuaChunkEntity& entity_data) {
        return ChunkEntityFactory::entity_bounds(entity_data);
    }
};

template<typename EntityType>
void EntityFactory<EntityType>::finalize(Camera *camera) {
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
                    EntityType *entity_data = entity.template get_mut<EntityType>();
                    Rect bounds = entity_bounds(*entity_data);
                    if (bounds.intersects(camera_bounds))
                        copy_vec.push_back(entity);
                }
            }
            // Sort entities in layer by y-axis
            for (auto &pair : copy_layer)
                std::sort(pair.second.begin(), pair.second.end(), [](flecs::entity a, flecs::entity b) {
                    EntityType *ra = a.template get_mut<EntityType>();
                    EntityType *rb = b.template get_mut<EntityType>();
                    return ra->y < rb->y;
                });
        }
    }
    
    // Pre-fetch textures to avoid holding texture lock during batch operations
    std::unordered_map<uint32_t, Texture*> texture_cache;
    for (auto [layer, v] : map_copy)
        for (auto [texture_id, vv] : v) {
            Texture* texture = $Entities.get_texture_by_id(texture_id);
            if (texture)
                texture_cache[texture_id] = texture;
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
