//
//  world.hpp
//  ice
//
//  Created by George Watson on 18/08/2025.
//

#pragma once

#include "chunk.hpp"
#include "camera.hpp"
#include "jobs.hpp"
#include "texture.hpp"
#include "fmt/format.h"
#include <unordered_map>
#include <shared_mutex>
#include <iostream>
#include "flecs.h"
#include "flecs_lua.h"
#include "renderable_manager.hpp"
#include "imgui.h"
#include "sol/sol_imgui.h"

class World {
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
    sg_pipeline _renderables_pipeline;

    flecs::world *_world = nullptr;
    lua_State *L = nullptr;

    static void _abort(void) {
        std::cerr << "ECS: ecs_os_abort() was called!\n";
        std::cerr.flush();
    }

    static void _log(int32_t level, const char *file, int32_t line, const char *msg) {
        FILE *stream;
        if (level >= 0)
            stream = stdout;
        else
            stream = stderr;

        if (level >= 0) {
            if (ecs_os_api.flags_ & EcsOsApiLogWithColors)
                fputs(ECS_MAGENTA, stream);
            fputs("info", stream);
        } else if (level == -2) {
            if (ecs_os_api.flags_ & EcsOsApiLogWithColors)
                fputs(ECS_YELLOW, stream);
            fputs("warning", stream);
        } else if (level == -3) {
            if (ecs_os_api.flags_ & EcsOsApiLogWithColors)
                fputs(ECS_RED, stream);
            fputs("error", stream);
        } else if (level == -4) {
            if (ecs_os_api.flags_ & EcsOsApiLogWithColors)
                fputs(ECS_RED, stream);
            fputs("fatal", stream);
        }

        if (ecs_os_api.flags_ & EcsOsApiLogWithColors)
            fputs(ECS_NORMAL, stream);
        fputs(": ", stream);

        if (level >= 0)
            if (ecs_os_api.log_indent_) {
                char indent[32];
                int i;
                for (i = 0; i < ecs_os_api.log_indent_; i ++) {
                    indent[i * 2] = '|';
                    indent[i * 2 + 1] = ' ';
                }
                indent[i * 2] = '\0';
                fputs(indent, stream);
            }

        if (level < 0) {
            if (file && line)
                fprintf(stream, "%s(%d): ", file, line);
            else if(file) {
                const char *file_ptr = strrchr(file, '/');
                if (!file_ptr)
                    file_ptr = strrchr(file, '\\');
                if (file_ptr)
                    file = file_ptr + 1;

                fputs(file, stream);
                fputs(": ", stream);
            }
            else if(line)
                fprintf(stream, "(%d): ", line);
        }

        fputs(msg, stream);
        fputs("\n", stream);
    }

    void ensure_chunk(int x, int y, bool priority) {
        uint64_t idx = index(x, y);

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
                                     Chunk::visibility_to_string(last_visibility),
                                     Chunk::visibility_to_string(new_visibility));
            if (new_visibility == ChunkVisibility::OutOfSign) {
                _chunks_being_destroyed.insert(chunk->id());
                chunk->mark_destroyed();
            }
        }
        // TODO: Add timer to Occulded chunks, if not visible for x seconds mark for deletion
        //       If chunk becomes visible again it will save time reloading it
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

        bool force_update_mvp = _camera->is_dirty();
        for (const auto& [id, chunk] : valid_chunks) {
            // Double-check chunk is still valid (avoid race condition)
            if (_chunks_being_destroyed.contains(id))
                continue;
//            if (chunk->id() != 0)
//                continue;
            sg_apply_pipeline(_pipeline);
            chunk->draw(force_update_mvp);
        }
    }

public:
    World(Camera *camera): _camera(camera),
    _create_chunk_queue([&](std::pair<int, int> coords) {
        auto [x, y] = coords;
        uint64_t idx = index(x, y);
        Chunk *chunk = new Chunk(x, y, _camera, _tilemap);

        {
            std::unique_lock<std::shared_mutex> lock(_chunks_lock);
            std::cout << fmt::format("New chunk created at ({}, {})\n", x, y);
            _chunks[idx] = chunk;
            _chunks_being_created.erase(idx);  // Remove from being created set
        }

        chunk->fill();
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
            .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8
        };
        _pipeline = sg_make_pipeline(&desc);
        desc.colors[0] = {
            .pixel_format = SG_PIXELFORMAT_RGBA8,
            .blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .src_factor_alpha = SG_BLENDFACTOR_ONE,
                .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
            }
        };
        _renderables_pipeline = sg_make_pipeline(&desc);
        _tilemap = $Assets.get<Texture>("tilemap.exploded.png");

        ecs_os_set_api_defaults();
        ecs_os_api_t os_api = ecs_os_api;
        os_api.abort_ = _abort;
        os_api.log_ = _log;
        ecs_os_set_api(&os_api);
        ecs_log_enable_colors(false);

        _world = new flecs::world();
        $Renderables.set_world(_world);
        // FlecsLua still uses C API for import
        ECS_IMPORT(_world->c_ptr(), FlecsLua);
        // Import C++ modules
#define X(MODULE) _world->import<MODULE>();
        MODULES
#undef X
        L = ecs_lua_get_state(_world->c_ptr());
        ecs_assert(L != NULL, ECS_INTERNAL_ERROR, NULL);

        luaL_openlibs(L);
        lua_getglobal(L, "package");
        lua_getfield(L, -1, "path");
        std::string current_path = lua_tostring(L, -1);
        std::string new_path = current_path + ";./scripts/?.lua";
        lua_pop(L, 1);
        lua_pushstring(L, new_path.c_str());
        lua_setfield(L, -2, "path");
        lua_pop(L, 1);

        sol::state_view lua(L);
        lua["imgui"] = imgui::load(static_cast<sol::state&>(lua));

        // Register texture function for Lua
        lua_register(L, "register_texture", [](lua_State* L) -> int {
            const char* path = luaL_checkstring(L, 1);
            uint32_t texture_id = $Renderables.register_texture(path);
            lua_pushinteger(L, texture_id);
            return 1; // Return the texture ID
        });

        lua_register(L, "framebuffer_width", [](lua_State* L) -> int {
            lua_pushinteger(L, framebuffer_width());
            return 1;
        });

        lua_register(L, "framebuffer_height", [](lua_State* L) -> int {
            lua_pushinteger(L, framebuffer_height());
            return 1;
        });

        lua_register(L, "framebuffer_resize", [](lua_State* L) -> int {
            int width = luaL_checkinteger(L, 1);
            int height = luaL_checkinteger(L, 2);
            framebuffer_resize(width, height);
            return 0; // No return value
        });

        lua_register(L, "chunk_index", [](lua_State* L) -> int {
            int x = luaL_checkinteger(L, 1);
            int y = luaL_checkinteger(L, 2);
            uint64_t result = index(x, y);
            lua_pushinteger(L, result);
            return 1;
        });

        auto unindex_func = [](lua_State* L) -> int {
            uint64_t i = luaL_checkinteger(L, 1);
            auto coords = unindex(i);
            lua_pushinteger(L, coords.first);
            lua_pushinteger(L, coords.second);
            return 2; // Return two values: x, y
        };
        lua_register(L, "chunk_unindex", unindex_func);

        const char *path = "scripts/test.lua";
        int result = luaL_dofile(L, path);
        if (result != LUA_OK) {
            const char* error_msg = lua_tostring(L, -1);
            fprintf(stderr, "Lua error loading %s: %s\n", path, error_msg);
            lua_pop(L, 1);
        }
        ecs_assert(L != NULL, ECS_INTERNAL_ERROR, NULL);
    }

    ~World() {
        $Renderables.clear();
        if (_world)
            delete _world;
        if (sg_query_shader_state(_shader) == SG_RESOURCESTATE_VALID)
            sg_destroy_shader(_shader);
        if (sg_query_pipeline_state(_pipeline) == SG_RESOURCESTATE_VALID)
            sg_destroy_pipeline(_pipeline);
        if (sg_query_pipeline_state(_renderables_pipeline) == SG_RESOURCESTATE_VALID)
            sg_destroy_pipeline(_renderables_pipeline);
        {
            std::unique_lock<std::shared_mutex> lock(_chunks_lock);
            for (auto& [id, chunk] : _chunks)
                if (chunk != nullptr)
                    delete chunk;
            _chunks.clear();
        }
    }

    bool update(float dt) {
        // TODO: modify these functions to also update $Renderables at same time as chunks
        update_chunks();
        scan_for_chunks();
        release_chunks();
        bool result = _world->progress(dt);
        if (result) {
            $Renderables.finalize(_camera);
            draw_chunks();
            sg_apply_pipeline(_renderables_pipeline);
            $Renderables.flush(_camera);
        }
        return result;
    }
};
