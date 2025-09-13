//
//  entity_factory.hpp
//  nice
//
//  Created by George Watson on 12/09/2025.
//

#pragma once

#include <iostream>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
#include <atomic>
#include "job_queue.hpp"
#include "vertex_batch.hpp"
#include "texture.hpp"
#include "glm/vec2.hpp"
#include "registrar.hpp"
#include "texture.hpp"

struct BasicVertex {
    glm::vec2 position;
    glm::vec2 texcoord;
};

template<typename EntityType>
class EntityFactory {
protected:
    using EntityMap = std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::vector<flecs::entity>>>;
    using BatchMap = std::unordered_map<uint32_t, std::unordered_map<uint32_t, VertexBatch<BasicVertex>>>;
    using EntityMapCache = std::unordered_map<flecs::entity, std::pair<uint32_t, uint32_t>>;

    struct DrawCall {
        VertexBatch<BasicVertex>* batch;
        std::vector<EntityType> entity_data; // Pre-extracted entity data to avoid accessing entities in worker thread
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
            for (const auto& entity_data : call.entity_data) {
                if (!call.texture) // Safety check
                    continue;
                // Create a mutable copy since generate_quad expects a non-const pointer
                EntityType mutable_entity_data = entity_data;
                BasicVertex *vertices = generate_quad(&mutable_entity_data, call.texture);
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

    static Rect entity_bounds(const EntityType& entity_data) {
        return Rect(static_cast<int>(entity_data.x - (entity_data.width * entity_data.scale_x * 0.5f)),
                    static_cast<int>(entity_data.y - (entity_data.height * entity_data.scale_y * 0.5f)),
                    static_cast<int>(entity_data.width * entity_data.scale_x),
                    static_cast<int>(entity_data.height * entity_data.scale_y));
    }

    void flush(Camera *camera=nullptr) {
        // Wait for all build jobs to complete before flushing
        wait_for_jobs_completion();
        {
            std::lock_guard<std::mutex> batch_lock(_batches_lock);
            for (auto &[layer, layer_batches] : batches)
                for (auto &[texture_id, batch] : layer_batches) {
                    if (batch.empty() || !batch.is_ready())
                        continue;
                    vs_params_t vs_params = { .mvp = camera ? camera->matrix() : glm::ortho(0.f, (float)framebuffer_width(), (float)framebuffer_height(), 0.f, -1.f, 1.f) };
                    sg_range params = SG_RANGE(vs_params);
                    sg_apply_uniforms(UB_vs_params, &params);
                    batch.flush();
                }
        }
        batches.clear();
    }

    virtual void update_entity(flecs::entity entity, EntityType &entity_data, bool lock=true) {
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

    void finalize(Registrar<Texture>* texture_registrar, Camera *camera=nullptr) {
        Rect camera_bounds = camera ? camera->bounds() : Rect{0,0,framebuffer_width(), framebuffer_height()};
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
                for (auto &pair : copy_layer) {
                    // Pre-extract y-coordinates to avoid accessing entities during sort
                    std::vector<std::pair<flecs::entity, float>> entities_with_y;
                    entities_with_y.reserve(pair.second.size());
                    
                    for (const auto& entity : pair.second) {
                        EntityType *entity_data = entity.template get_mut<EntityType>();
                        entities_with_y.emplace_back(entity, entity_data->y);
                    }
                    
                    // Sort using pre-extracted y values
                    std::sort(entities_with_y.begin(), entities_with_y.end(), 
                        [](const auto& a, const auto& b) {
                            return a.second < b.second;
                        });
                    
                    // Extract sorted entities back to the original vector
                    pair.second.clear();
                    pair.second.reserve(entities_with_y.size());
                    for (const auto& entity_y_pair : entities_with_y)
                        pair.second.push_back(entity_y_pair.first);
                }
            }
        }

        // Pre-fetch textures to avoid holding texture lock during batch operations
        std::unordered_map<uint32_t, Texture*> texture_cache;
        for (auto [layer, v] : map_copy)
            for (auto [texture_id, vv] : v) {
                Texture* texture = texture_registrar->get_asset(texture_id);
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
                
                // Pre-extract entity data to avoid accessing entities in worker thread
                std::vector<EntityType> entity_data_list;
                entity_data_list.reserve(vv.size());
                for (const auto& entity : vv) {
                    if (!entity.is_alive())
                        continue;
                    EntityType *entity_data = entity.template get_mut<EntityType>();
                    entity_data_list.push_back(*entity_data);
                }
                
                _pending_jobs.fetch_add(1);
                _build_queue.enqueue(DrawCall{batch, std::move(entity_data_list), texture});
            }
        }
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

    std::shared_mutex& get_lock() {
        return _entities_lock;
    }
};
