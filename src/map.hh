//
//  map.hh
//  rpg
//
//  Created by George Watson on 04/08/2025.
//

#pragma once

#include "chunk.hh"
#include "job.hh"
#include <unordered_map>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <fmt/format.h>
#include "flecs.h"

class Map {
    ThreadPool _worker{std::thread::hardware_concurrency()};
    std::unordered_map<uint64_t, Chunk*> _chunks;
    mutable std::shared_mutex _chunks_mutex;
    std::vector<uint64_t> _delete_queue;
    mutable std::mutex _delete_queue_mutex;
    JobQueue<Chunk*> _vertex_generator;
    JobQueue<std::pair<Chunk*, ChunkVertex*>> _mesh_builder;

    flecs::world *_ecs;
    Camera *_camera;
    Texture *_tilemap;
    sg_shader _shader;
    sg_pipeline _pipeline; 

    bool update_chunk(Chunk *chunk, const Rect &camera_bounds, const Rect &max_bounds) {
        ChunkState old_state = chunk->state();
        chunk->set_state(!max_bounds.intersects(chunk->bounds()) ?
                         ChunkState::CHUNK_STATE_UNLOAD :
                         camera_bounds.intersects(chunk->bounds()) ? ChunkState::CHUNK_STATE_ACTIVE :
                                                                         ChunkState::CHUNK_STATE_DORMANT);
        bool result = chunk->state() != old_state;
        if (result)
            std::cout << fmt::format("Chunk state changed at ({}, {}) from {} to {}\n",
                                static_cast<int>(chunk->position().x), static_cast<int>(chunk->position().y),
                                chunk_state_to_string(old_state), chunk_state_to_string(chunk->state()));
        if (chunk->state() != ChunkState::CHUNK_STATE_UNLOAD && chunk->is_dirty() && chunk->is_filled()) {
            std::cout << fmt::format("Chunk at ({}, {}) queued for build\n",
                                     static_cast<int>(chunk->position().x), static_cast<int>(chunk->position().y));
            chunk->mark_clean();
            _vertex_generator.push(chunk);
        }
        return result;
    }

    void check_chunks() {
        std::shared_lock<std::shared_mutex> chunks_lock(_chunks_mutex);
        std::lock_guard<std::mutex> delete_lock(_delete_queue_mutex);

        Rect camera_bounds = _camera->bounds();
        Rect max_bounds = _camera->max_bounds();
        for (auto it = _chunks.begin(); it != _chunks.end(); ++it) {
            Chunk *chunk = it->second;
            if (update_chunk(chunk, camera_bounds, max_bounds) && chunk->state() == ChunkState::CHUNK_STATE_UNLOAD)
                _delete_queue.push_back(chunk->id());
        }
    }

    void release_chunks() {
        std::lock_guard<std::mutex> delete_lock(_delete_queue_mutex);

        for (uint64_t id : _delete_queue) {
            std::unique_lock<std::shared_mutex> chunks_lock(_chunks_mutex);
            Chunk *chunk = _chunks[id];
            _chunks.erase(id);
            flecs::entity chunk_entity = _ecs->entity(fmt::format("Chunk({}, {})", chunk->position().x, chunk->position().y).c_str());
            _ecs->defer_begin();
            chunk_entity.destruct();
            _ecs->defer_end();
            chunks_lock.unlock();
            std::cout << fmt::format("Chunk at ({}, {}) released\n",
                                     static_cast<int>(chunk->position().x), static_cast<int>(chunk->position().y));
            delete chunk;
        }
        _delete_queue.clear();
    }

    Chunk* ensure_chunk(int x, int y, bool create = false) {
        uint64_t idx = Chunk::id(x, y);
        {
            std::shared_lock<std::shared_mutex> chunks_lock(_chunks_mutex);
            if (_chunks.find(idx) != _chunks.end())
                return _chunks[idx];
        }
        if (!create)
            return nullptr;
        std::unique_lock<std::shared_mutex> chunks_lock(_chunks_mutex);
        if (_chunks.find(idx) != _chunks.end())
            return _chunks[idx];

        // TODO: Search on disk for chunk data
        std::cout << fmt::format("Creating chunk at ({}, {})\n", x, y);
        Chunk *chunk = new Chunk(x, y, _tilemap);
        _chunks[idx] = chunk;
        flecs::entity chunk_entity = _ecs->entity(fmt::format("Chunk({}, {})", x, y).c_str())
                                         .set<ChunkEntity>(ChunkEntity{chunk});

        chunks_lock.unlock();
        _worker.enqueue(_camera->bounds().intersects(chunk->bounds()), [chunk]() {
            std::cout << fmt::format("Filling chunk at ({}, {})\n",
                                     static_cast<int>(chunk->position().x), static_cast<int>(chunk->position().y));
            chunk->fill();
        });
        return chunk;
    }

    void find_chunks() {
        Rect bounds = _camera->max_bounds();
        glm::vec2 tl = glm::vec2(bounds.x, bounds.y);
        glm::vec2 br = glm::vec2(bounds.x + bounds.w, bounds.y + bounds.h);
        glm::vec2 tl_chunk = _camera->world_to_chunk(tl);
        glm::vec2 br_chunk = _camera->world_to_chunk(br);
        for (int y = (int)tl_chunk.y; y <= (int)br_chunk.y; y++)
            for (int x = (int)tl_chunk.x; x <= (int)br_chunk.x; x++)
                if (Chunk::bounds(x, y).intersects(bounds))
                    assert(ensure_chunk(x, y, true) != NULL);
    }

    int draw_chunks() {
        sg_apply_pipeline(_pipeline);
        int i = 0;
        glm::mat4 *mvp = nullptr;
        if (_camera->dirty) {
            mvp = new glm::mat4();
            glm::mat4 camera_matrix = _camera->matrix();
            memcpy(mvp, &camera_matrix, sizeof(glm::mat4));
        }

        // Use shared lock for reading chunks during drawing
        std::shared_lock<std::shared_mutex> chunks_lock(_chunks_mutex);
        for (auto it = _chunks.begin(); it != _chunks.end(); ++it) {
            Chunk *chunk = it->second;
            if (chunk->draw(mvp))
                i++;
        }

        if (mvp != nullptr)
            delete mvp;
        return i;
    }

public:
    Map(flecs::world *ecs, Camera *camera, Texture *tilemap): 
        _ecs(ecs), _camera(camera), _tilemap(tilemap),
        _vertex_generator([this](Chunk* chunk) {
            std::cout << fmt::format("Generating vertices for chunk at ({}, {})\n",
                                     static_cast<int>(chunk->position().x), static_cast<int>(chunk->position().y));
            ChunkVertex* vertices = chunk->vertices();
            _mesh_builder.push({chunk, vertices});
        }),
        _mesh_builder([](std::pair<Chunk*, ChunkVertex*> data) {
            auto [chunk, vertices] = data;
            std::cout << fmt::format("Building chunk at ({}, {})\n",
                                     static_cast<int>(chunk->position().x), static_cast<int>(chunk->position().y));
            chunk->build(vertices);
            delete[] vertices;
            std::cout << fmt::format("Finished building chunk at ({}, {})\n",
                                     static_cast<int>(chunk->position().x), static_cast<int>(chunk->position().y));
        }) {
        _shader = sg_make_shader(basic_shader_desc(sg_query_backend()));
        sg_pipeline_desc desc = {
            .shader = _shader,
            .layout = {
                .buffers[0].stride = sizeof(ChunkVertex),
                .attrs = {
                    [ATTR_basic_position].format = SG_VERTEXFORMAT_FLOAT2,
                    [ATTR_basic_texcoord].format = SG_VERTEXFORMAT_FLOAT2
                }
            },
            .depth = {
                .pixel_format = SG_PIXELFORMAT_DEPTH,
                .compare = SG_COMPAREFUNC_LESS_EQUAL,
                .write_enabled = true
            },
            .cull_mode = SG_CULLMODE_BACK,
            .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8
        };
        _pipeline = sg_make_pipeline(&desc);

        flecs::system draw = _ecs->system<ChunkEntity>().each([this](ChunkEntity &chunk_entity) {
            int n = this->draw_chunks();
            if (n > 0)
                this->_camera->dirty = false;
        });
    }

    ~Map() {
        if (sg_query_pipeline_state(_pipeline) == SG_RESOURCESTATE_VALID)
            sg_destroy_pipeline(_pipeline);
        if (sg_query_shader_state(_shader) == SG_RESOURCESTATE_VALID)
            sg_destroy_shader(_shader);
    }

    Camera* camera() const {
        return _camera;
    }

    void update() {
        check_chunks();
        release_chunks();
        find_chunks();
    }
};
