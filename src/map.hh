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

class Map {
    std::unordered_map<uint64_t, Chunk*> _chunks;
    std::vector<uint64_t> _delete_queue;
    Camera *_camera;
    Texture *_tilemap;
    ThreadPool _worker{std::thread::hardware_concurrency()};
    sg_shader _shader;
    sg_pipeline _pipeline;

    bool update_chunk(Chunk *chunk) {
        ChunkState old_state = chunk->state();
        chunk->set_state(!_camera->max_bounds().intersects(chunk->bounds()) ?
                         ChunkState::CHUNK_STATE_UNLOAD :
                         _camera->bounds().intersects(chunk->bounds()) ? ChunkState::CHUNK_STATE_ACTIVE :
                                                                         ChunkState::CHUNK_STATE_DORMANT);
        bool result = chunk->state() != old_state;
        if (result)
            printf("CHUNK MODIFIED (%d, %d) (%s -> %s)\n",
                   static_cast<int>(chunk->position().x), static_cast<int>(chunk->position().y),
                   chunk_state_to_string(old_state), chunk_state_to_string(chunk->state()));
        return result;
    }

    void check_chunks() {
        for (auto it = _chunks.begin(); it != _chunks.end(); ++it) {
            Chunk *chunk = it->second;
            if (update_chunk(chunk) && chunk->state() == ChunkState::CHUNK_STATE_UNLOAD)
                _delete_queue.push_back(chunk->id());
        }
    }

    void release_chunks() {
        for (uint64_t id : _delete_queue) {
            Chunk *chunk = _chunks[id];
            _chunks.erase(id);
            delete chunk;
            printf("CHUNK RELEASED (%d, %d)\n", static_cast<int>(chunk->position().x), static_cast<int>(chunk->position().y));
        }
        _delete_queue.clear();
    }

    Chunk* ensure_chunk(int x, int y, bool create = false) {
        uint64_t idx = Chunk::id(x, y);
        if (_chunks.find(idx) != _chunks.end())
            return _chunks[idx];
        if (!create)
            return nullptr;
        // TODO: Search on disk for chunk data
        printf("CHUNK CREATED (%d, %d)\n", x, y);
        Chunk *chunk = new Chunk(x, y, _tilemap);
        // Simply run the function in the background - no future handling needed
        _worker.enqueue(_camera->bounds().intersects(chunk->bounds()), [chunk]() {
            printf("FILLING CHUNK (%d, %d)\n", static_cast<int>(chunk->position().x), static_cast<int>(chunk->position().y));
            chunk->fill();
        });
        _chunks[idx] = chunk;
        return chunk;
    }

    void find_chunks() {
        Rect bounds = _camera->max_bounds();
        glm::vec2 tl = _camera->screen_to_world(glm::vec2(bounds.x, bounds.y));
        glm::vec2 br = _camera->screen_to_world(glm::vec2(bounds.x + bounds.w, bounds.y + bounds.h));
        glm::vec2 tl_chunk = _camera->world_to_chunk(tl);
        glm::vec2 br_chunk = _camera->world_to_chunk(br);
        for (int y = (int)tl_chunk.y; y <= (int)br_chunk.y; y++)
            for (int x = (int)tl_chunk.x; x <= (int)br_chunk.x; x++)
                if (Chunk::bounds(x, y).intersects(bounds))
                    assert(ensure_chunk(x, y, true) != NULL);
    }

    void draw_chunks() {
        sg_apply_pipeline(_pipeline);
        int i = 0;
        for (auto it = _chunks.begin(); it != _chunks.end(); ++it) {
            Chunk *chunk = it->second;
            chunk->draw(const_cast<Camera*>(_camera));
            i++;
        }
        printf("CHUNKS DRAWN: %d\n", i);
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

    void draw() {
        check_chunks();
        release_chunks();
        find_chunks();
        draw_chunks();
        _camera->dirty = false;
    }
};
