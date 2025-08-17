//
//  chunk_factory.hh
//  rpg
//
//  Created by George Watson on 14/08/2025.
//

#pragma once

#include "assets.hh"
#include "chunk.hh"
#include "jobs.hh"
#include <unordered_map>
#include <iostream>
#include "fmt/format.h"
#include "robot_factory.hh"

typedef VertexBatch<RobotVertex> RobotVertexBatch;

class ChunkFactory {
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

    RobotFactory *_robot_factory;
    RobotVertexBatch _robot_batch;

    void ensure_chunk(int x, int y, bool priority) {
        uint64_t idx = Chunk::id(x, y);
        
        // Check if chunk already exists or is being processed
        {
            std::shared_lock<std::shared_mutex> chunks_lock(_chunks_lock);
            if (_chunks.find(idx) != _chunks.end())
                return;
        }
        
        // Check and insert atomically using ThreadSafeSet methods
        if (_chunks_being_created.contains(idx) || 
            _chunks_being_built.contains(idx))
            return;

        // Try to mark as being created
        if (!_chunks_being_created.insert(idx))
            return; // Another thread already marked it

        if (priority)
            _create_chunk_queue.enqueue_priority({x, y});
        else
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
            if (new_visibility == ChunkVisibility::OutOfSign) {
                _chunks_being_destroyed.insert(chunk->id());
                chunk->mark_destroyed();
            }
        }
        if (new_visibility != ChunkVisibility::OutOfSign)
            _robot_factory->update_robots(Chunk::id(chunk->x(), chunk->y()));
    }

public:
    ChunkFactory(Camera *camera): _camera(camera),
    _create_chunk_queue([&](std::pair<int, int> coords) {
        auto [x, y] = coords;
        uint64_t idx = Chunk::id(x, y);
        Chunk *chunk = new Chunk(x, y, _tilemap);
        
        {
            std::unique_lock<std::shared_mutex> lock(_chunks_lock);
            std::cout << fmt::format("New chunk created at ({}, {})\n", x, y);
            _chunks[idx] = chunk;
            _chunks_being_created.erase(idx);  // Remove from being created set
        }
        
        chunk->fill();
        _robot_factory->add_robots(idx, chunk->poisson(5));
        std::cout << fmt::format("Chunk at ({}, {}) finished filling\n", x, y);

        {
            std::lock_guard<std::shared_mutex> lock(_chunks_lock);
            _chunks_being_built.insert(idx);  // Mark as being built
        }
        _build_chunk_queue.enqueue(chunk);
    }),
    _build_chunk_queue([&](Chunk *chunk) {
        chunk->build();
        std::cout << fmt::format("Chunk at ({}, {}) finished building\n", chunk->x(), chunk->y());

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
            .colors[0] = {
                .pixel_format = SG_PIXELFORMAT_RGBA8,
                .blend = {
                    .enabled = true,
                    .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                    .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .src_factor_alpha = SG_BLENDFACTOR_ONE,
                    .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
                }
            }
        };
        _pipeline = sg_make_pipeline(&desc);
        _tilemap = $Assets.get<Texture>("tilemap");

        _robot_batch.set_texture($Assets.get<Texture>("robot"));
        _robot_factory = new RobotFactory(_camera);
    }

    ~ChunkFactory() {
        if (sg_query_shader_state(_shader) == SG_RESOURCESTATE_VALID)
            sg_destroy_shader(_shader);
        if (sg_query_pipeline_state(_pipeline) == SG_RESOURCESTATE_VALID)
            sg_destroy_pipeline(_pipeline);
        {
            std::unique_lock<std::shared_mutex> lock(_chunks_lock);
            for (auto& [id, chunk] : _chunks)
                if (chunk != nullptr)
                    delete chunk;
            _chunks.clear();
        }
    }

    void update_chunks() {
        Rect max_bounds = _camera->max_bounds();
        Rect bounds = _camera->bounds();
        
        // Collect chunks to update without holding the lock
        std::vector<Chunk*> chunks_to_update;
        {
            std::shared_lock<std::shared_mutex> lock(_chunks_lock);
            chunks_to_update.reserve(_chunks.size());
            for (const auto& [id, chunk] : _chunks)
                chunks_to_update.push_back(chunk);
        }
        
        // Update chunks without holding the lock
        for (auto chunk : chunks_to_update)
            update_chunk(chunk, bounds, max_bounds);
    }

    void scan_for_chunks() {
        Rect max_bounds = _camera->max_bounds();
        Rect bounds = _camera->bounds();
        glm::vec2 tl = glm::vec2(max_bounds.x, max_bounds.y);
        glm::vec2 br = glm::vec2(max_bounds.x + max_bounds.w, max_bounds.y + max_bounds.h);
        glm::vec2 tl_chunk = _camera->world_to_chunk(tl);
        glm::vec2 br_chunk = _camera->world_to_chunk(br);
        for (int y = (int)tl_chunk.y; y <= (int)br_chunk.y; y++)
            for (int x = (int)tl_chunk.x; x <= (int)br_chunk.x; x++) {
                Rect chunk_bounds = Chunk::bounds(x, y);
                if (chunk_bounds.intersects(max_bounds))
                    ensure_chunk(x, y, chunk_bounds.intersects(bounds));
            }
    }

    void release_chunks() {
        std::vector<uint64_t> chunks_to_destroy;
        std::vector<Chunk*> chunks_to_delete;
        {
            std::unique_lock<std::shared_mutex> lock(_chunks_lock);
            // Iterate through chunks and check if they're marked for destruction
            for (auto it = _chunks.begin(); it != _chunks.end();) {
                uint64_t chunk_id = it->first;
                Chunk* chunk = it->second;
                
                if (_chunks_being_destroyed.contains(chunk_id)) {
                    std::cout << fmt::format("Releasing chunk at ({}, {})\n", chunk->x(), chunk->y());
                    chunks_to_delete.push_back(chunk);
                    chunks_to_destroy.push_back(chunk_id);
                    it = _chunks.erase(it);
                } else
                    ++it;
            }
        }
        
        // Delete chunks outside of the lock
        for (Chunk* chunk : chunks_to_delete)
            delete chunk;
        // Remove from destroyed set
        for (uint64_t chunk_id : chunks_to_destroy)
            _chunks_being_destroyed.erase(chunk_id);
    }

    void draw_chunks() {
        // Collect valid chunks without holding the lock for too long
        std::vector<std::pair<uint64_t, Chunk*>> valid_chunks;
        {
            std::shared_lock<std::shared_mutex> lock(_chunks_lock);
            valid_chunks.reserve(_chunks.size());
            for (const auto& [id, chunk] : _chunks)
                if (chunk != nullptr && !_chunks_being_destroyed.contains(id))
                    valid_chunks.emplace_back(id, chunk);
        }

        // Draw chunks first
        sg_apply_pipeline(_pipeline);
        bool force_update_mvp = _camera->is_dirty();
        for (const auto& [id, chunk] : valid_chunks) {
            // Double-check chunk is still valid (avoid race condition)
            if (_chunks_being_destroyed.contains(id))
                continue;
                
            chunk->draw(_camera, force_update_mvp);
            size_t vertex_count;
            RobotVertex *vertices = _robot_factory->vertices(id, &vertex_count);
            if (vertices && vertex_count > 0) {
                _robot_batch.add_vertices(vertices, vertex_count);
                delete[] vertices;
            }
        }

        if (_robot_batch.build()) {
            sg_apply_pipeline(_pipeline);
            vs_params_t vs_params = { .mvp = _camera->matrix() };
            sg_range params = SG_RANGE(vs_params);
            sg_apply_uniforms(UB_vs_params, &params);
            _robot_batch.flush(true);
        }
    }
};
