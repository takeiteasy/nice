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

    static Rect entity_bounds(const EntityType& entity_data) {
        return Rect(static_cast<int>(entity_data.x - (entity_data.scale_x * 0.5f)),
                    static_cast<int>(entity_data.y - (entity_data.scale_y * 0.5f)),
                    static_cast<int>(entity_data.width * entity_data.scale_x),
                    static_cast<int>(entity_data.height * entity_data.scale_y));
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
