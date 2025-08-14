//
//  chunk_manager.hh
//  rpg
//
//  Created by George Watson on 14/08/2025.
//

#pragma once

#include "chunk.hh"
#include "jobs.hh"
#include "basic.glsl.h"
#include <unordered_map>
#include <iostream>
#include "fmt/format.h"

class ChunkManager {
    std::unordered_map<uint64_t, Chunk*> _chunks;
    mutable std::shared_mutex _chunks_lock;
    JobQueue<std::pair<int, int>> _create_chunk_queue;
    ThreadSafeSet<uint64_t> _chunks_being_created;
    JobQueue<Chunk*> _build_chunk_queue;
    ThreadSafeSet<uint64_t> _chunks_being_built;
    ThreadSafeSet<uint64_t> _chunks_being_destroyed;

    Camera *_camera;
    Texture *_tilemap;
    sg_shader _shader;
    sg_pipeline _pipeline;

    void ensure_chunk(int x, int y) {
        uint64_t idx = Chunk::id(x, y);
        
        // Check if chunk already exists or is being created
        std::unique_lock<std::shared_mutex> chunks_lock(_chunks_lock);
        auto created_lock = _chunks_being_created.get_shared_lock();
        auto built_lock = _chunks_being_built.get_shared_lock();
        
        if (_chunks.find(idx) != _chunks.end() ||
            _chunks_being_created.contains_unsafe(idx) ||
            _chunks_being_built.contains_unsafe(idx)) {
            return;
        }
        
        // Upgrade to exclusive lock for _chunks_being_created to modify it
        created_lock.unlock();
        built_lock.unlock();
        
        // Mark as being created
        _chunks_being_created.insert(idx);
        chunks_lock.unlock();
        _create_chunk_queue.enqueue({x, y});
    }

    void update_chunk(Chunk *chunk, const Rect &camera_bounds, const Rect &max_bounds) {
        if (!chunk->is_ready())
            return;

        ChunkVisibility last_visibility = chunk->visibility();
        Rect chunk_bounds = chunk->bounds();
        ChunkVisibility new_visibility = !max_bounds.intersects(chunk_bounds) ? ChunkVisibility::OutOfSign :
                                         camera_bounds.intersects(chunk_bounds) ? ChunkVisibility::Visible :
                                         ChunkVisibility::Occluded;
        chunk->set_visibility(new_visibility);
        if (new_visibility != last_visibility) {
            std::cout << fmt::format("Chunk at ({}, {}) visibility changed from {} to {}\n",
                            chunk->x(), chunk->y(),
                            chunk_visibility_to_string(last_visibility),
                            chunk_visibility_to_string(new_visibility));
            switch (new_visibility) {
                case ChunkVisibility::OutOfSign:
                {
                    _chunks_being_destroyed.insert(chunk->id());
                    break;
                }
                case ChunkVisibility::Visible:
                    break;
                case ChunkVisibility::Occluded:
                    break;
            }
        }
    }

public:
    ChunkManager(Camera *camera, Texture *tilemap): _camera(camera), _tilemap(tilemap),
    _create_chunk_queue([&](std::pair<int, int> coords) {
        auto [x, y] = coords;
        uint64_t idx = Chunk::id(x, y);
        Chunk *chunk = new Chunk(x, y, _tilemap);
        
        {
            std::unique_lock<std::shared_mutex> lock(_chunks_lock);
            std::cout << fmt::format("Creating chunk at ({}, {})\n", x, y);
            _chunks[idx] = chunk;
            _chunks_being_created.erase(idx);  // Remove from being created set
        }
        
        chunk->fill();
        std::cout << fmt::format("Chunk at ({}, {}) filled\n", x, y);

        {
            std::lock_guard<std::shared_mutex> lock(_chunks_lock);
            _chunks_being_built.insert(idx);  // Mark as being built
        }
        _build_chunk_queue.enqueue(chunk);
    }),
    _build_chunk_queue([&](Chunk *chunk) {
        chunk->build();
        std::cout << fmt::format("Chunk at ({}, {}) built\n", chunk->x(), chunk->y());
        
        // Remove from being built set after successful build
        uint64_t idx = chunk->id();
        _chunks_being_built.erase(idx);
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
    }

    ~ChunkManager() {
        if (sg_query_shader_state(_shader) == SG_RESOURCESTATE_VALID)
            sg_destroy_shader(_shader);
        if (sg_query_pipeline_state(_pipeline) == SG_RESOURCESTATE_VALID)
            sg_destroy_pipeline(_pipeline);
    }

    void update_chunks() {
        std::unique_lock<std::shared_mutex> lock(_chunks_lock);
        for (auto it = _chunks.begin(); it != _chunks.end();) {
            auto chunk = it->second;

        }
    }

    void scan_for_chunks() {
        Rect bounds = _camera->max_bounds();
        glm::vec2 tl = glm::vec2(bounds.x, bounds.y);
        glm::vec2 br = glm::vec2(bounds.x + bounds.w, bounds.y + bounds.h);
        glm::vec2 tl_chunk = _camera->world_to_chunk(tl);
        glm::vec2 br_chunk = _camera->world_to_chunk(br);
        for (int y = (int)tl_chunk.y; y <= (int)br_chunk.y; y++)
            for (int x = (int)tl_chunk.x; x <= (int)br_chunk.x; x++)
                if (Chunk::bounds(x, y).intersects(bounds))
                    ensure_chunk(x, y);
    }
};
