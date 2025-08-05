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
#include <chrono>
#include <fmt/format.h>

class Map {
    std::unordered_map<uint64_t, Chunk*> _chunks;
    std::vector<uint64_t> _delete_queue;
    Camera *_camera;
    Texture *_tilemap;
    ThreadPool _worker{std::thread::hardware_concurrency()};
    sg_shader _shader;
    sg_pipeline _pipeline;

    mutable std::shared_mutex _chunks_mutex;
    mutable std::mutex _delete_queue_mutex;

    static bool update_chunk(Chunk *chunk, const Rect &camera_bounds, const Rect &max_bounds) {
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
            // Need exclusive access to chunks map for deletion
            std::unique_lock<std::shared_mutex> chunks_lock(_chunks_mutex);
            Chunk *chunk = _chunks[id];
            _chunks.erase(id);
            chunks_lock.unlock(); // Release lock before delete to avoid holding it too long

            std::cout << fmt::format("Chunk at ({}, {}) released\n",
                                     static_cast<int>(chunk->position().x), static_cast<int>(chunk->position().y));
            delete chunk;
        }
        _delete_queue.clear();
    }

    Chunk* ensure_chunk(int x, int y, bool create = false) {
        uint64_t idx = Chunk::id(x, y);
        // First try with shared lock (read access)
        {
            std::shared_lock<std::shared_mutex> chunks_lock(_chunks_mutex);
            if (_chunks.find(idx) != _chunks.end())
                return _chunks[idx];
        }
        if (!create)
            return nullptr;

        // Need exclusive lock for chunk creation
        std::unique_lock<std::shared_mutex> chunks_lock(_chunks_mutex);

        // Double-check pattern - another thread might have created it
        if (_chunks.find(idx) != _chunks.end())
            return _chunks[idx];

        // TODO: Search on disk for chunk data
        std::cout << fmt::format("Creating chunk at ({}, {})\n", x, y);
        Chunk *chunk = new Chunk(x, y, _tilemap);
        _chunks[idx] = chunk;

        // Release the chunks lock before enqueueing work to avoid deadlock
        chunks_lock.unlock();

        // Simply run the function in the background - no future handling needed
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
    Map() {
        _camera = new Camera();
        _tilemap = new Texture("assets/tilemap.exploded.png");
        _shader = sg_make_shader(default_program_shader_desc(sg_query_backend()));
        sg_pipeline_desc desc = {
            .shader = _shader,
            .layout = {
                .buffers[0].stride = sizeof(ChunkVertex),
                .attrs = {
                    [ATTR_default_program_position].format = SG_VERTEXFORMAT_FLOAT2,
                    [ATTR_default_program_texcoord].format = SG_VERTEXFORMAT_FLOAT2
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

    int draw() {
        check_chunks();
        release_chunks();
        find_chunks();
        int n = draw_chunks();
        _camera->dirty = false;
        return n;
    }
};
