//
//  world.hpp
//  nice
//
//  Created by George Watson on 18/08/2025.
//

#pragma once

#include "chunk.hpp"
#include "chunk_manager.hpp"
#include "job_queue.hpp"
#include "texture.hpp"
#include "fmt/format.h"
#include <unordered_map>
#include <shared_mutex>
#include <iostream>
#include <queue>
#include <string>
#include <filesystem>
#include "just_zip.h"
#include "flecs.h"
#include "flecs_lua.h"
#include "chunk_entity.hpp"
#include "imgui.h"
#include "sol/sol_imgui.h"
#include "nice.dat.h"
#include "uuid.h"
#include "sokol/sokol_time.h"
#include "camera.hpp"
#include "input_manager.hpp"
#include "uuid.h"
#include "registrar.hpp"
#include "screen_entity.hpp"

class World {
    uuid::v4::UUID _id;

    Camera _camera;
    Texture *_tilemap;
    sg_shader _shader;
    sg_pipeline _pipeline;
    sg_pipeline _entity_pipeline;

    flecs::world *_world = nullptr;
    lua_State *L = nullptr;

    ChunkEntityFactory _chunk_entities;
    ScreenEntityFactory _screen_entities;
    Registrar<Texture> _texture_registry;

    static void _abort(void) {
        std::cerr << "ECS: ecs_os_abort() was called!\n";
        std::cerr.flush();
    }

    std::string _get_world_directory() {
        std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
        std::filesystem::path world_dir = temp_dir / _id.String();
        if (!std::filesystem::exists(world_dir))
            std::filesystem::create_directories(world_dir);
        return world_dir.string();
    }

    std::string _get_chunk_filepath(int x, int y) {
        uint64_t chunk_index = index(x, y);
        std::string filename = std::to_string(chunk_index) + ".nicechunk";
        std::filesystem::path world_dir = _get_world_directory();
        return (world_dir / filename).string();
    }

    void _export() {
        try {
            std::string archive_name = _id.String() + ".niceworld";
            std::cout << fmt::format("Creating world archive: {}\n", archive_name);
            zip* archive = zip_open(archive_name.c_str(), "w");
            if (!archive) {
                std::cout << fmt::format("Failed to create archive: {}\n", archive_name);
                return;
            }
            std::string world_dir = _get_world_directory();
            
            // Iterate through all .nicechunk files in the world directory
            for (const auto& entry : std::filesystem::directory_iterator(world_dir))
                if (entry.is_regular_file() && entry.path().extension() == ".nicechunk") {
                    std::string file_path = entry.path().string();
                    std::string filename = entry.path().filename().string();
                    FILE* file = fopen(file_path.c_str(), "rb");
                    if (!file) {
                        std::cout << fmt::format("Failed to open file for archiving: {}\n", file_path);
                        continue;
                    }
                    bool success = zip_append_file_ex(archive, file_path.c_str(), filename.c_str(), file, 6); // compression level 6
                    fclose(file);
                    if (success)
                        std::cout << fmt::format("Added {} to archive\n", filename);
                    else
                        std::cout << fmt::format("Failed to add {} to archive\n", filename);
                }
            
            zip_close(archive);
            std::cout << fmt::format("World archive created successfully: {}\n", archive_name);
            
            // Clean up the temporary directory after successful archiving
            std::filesystem::remove_all(world_dir);
            std::cout << fmt::format("Cleaned up temporary directory: {}\n", world_dir);
        } catch (const std::exception& e) {
            std::cout << fmt::format("Error creating world archive: {}\n", e.what());
        }
    }

    bool _import(const std::string& archive_path) {
        try {
            std::cout << fmt::format("Loading world from archive: {}\n", archive_path);
            zip* archive = zip_open(archive_path.c_str(), "r");
            if (!archive) {
                std::cout << fmt::format("Failed to open archive: {}\n", archive_path);
                return false;
            }
            std::string world_dir = _get_world_directory();
            unsigned count = zip_count(archive);
            std::cout << fmt::format("Archive contains {} files\n", count);
            
            for (unsigned i = 0; i < count; i++) {
                char* filename = zip_name(archive, i);
                if (filename && strstr(filename, ".nicechunk")) {
                    std::string output_path = (std::filesystem::path(world_dir) / filename).string();
                    FILE* output_file = fopen(output_path.c_str(), "wb");
                    if (!output_file) {
                        std::cout << fmt::format("Failed to create output file: {}\n", output_path);
                        continue;
                    }
                    bool success = zip_extract_file(archive, i, output_file);
                    fclose(output_file);
                    if (success)
                        std::cout << fmt::format("Extracted {} from archive\n", filename);
                    else
                        std::cout << fmt::format("Failed to extract {} from archive\n", filename);
                }
            }
            
            zip_close(archive);
            std::cout << fmt::format("World loaded successfully from archive\n");
        } catch (const std::exception& e) {
            std::cout << fmt::format("Error loading world from archive: {}\n", e.what());
            return false;
        }

        // Extract UUID from filename if possible
        std::filesystem::path p;
        std::string filename = std::filesystem::path(archive_path).filename().string();
        _id = filename.substr(0, filename.find('.'));
        // Loop through extracted chunk files and queue them
        std::string world_dir = _get_world_directory();
        for (const auto& entry : std::filesystem::directory_iterator(world_dir))
            if (entry.is_regular_file() && entry.path().extension() == ".nicechunk") {
                std::string file_path = entry.path().string();
                std::string fname = entry.path().filename().string();
                try {
                    uint64_t chunk_index = std::stoull(fname.substr(0, fname.find('.')));
                    auto [x, y] = unindex(chunk_index);
                    $Chunks.ensure_chunk(x, y, false);
                } catch (const std::exception& e) {
                    std::cout << fmt::format("Failed to parse chunk filename {}: {}\n", fname, e.what());
                    return false;
                }
            }
        return true;
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

    static World* get_world_from_lua(lua_State* L) {
        lua_getfield(L, LUA_REGISTRYINDEX, "__world__");
        World* world = static_cast<World*>(lua_touserdata(L, -1));
        lua_pop(L, 1); // Remove the userdata from stack
        if (!world) {
            std::cout << "World instance not found in Lua registry\n";
            return nullptr;
        }
        return world;
    }

    static const LuaChunkEntity* get_entity_from_lua(lua_State* L) {
        World* world = get_world_from_lua(L);
        if (!world) {
            std::cout << "Failed to get World instance from Lua\n";
            return nullptr;
        }
        uint64_t entity_id = static_cast<uint64_t>(luaL_checkinteger(L, 1));
        flecs::entity entity = world->_world->entity(entity_id);
        const LuaChunkEntity* entity_data = entity.get<LuaChunkEntity>();
        if (!entity_data) {
            std::cout << "ChunkEntity does not have LuaChunkEntity component\n";
            return nullptr;
        }
        return entity_data;
    }

    static LuaChunkEntity* get_mutable_entity_from_lua(lua_State* L) {
        World* world = get_world_from_lua(L);
        if (!world) {
            std::cout << "Failed to get World instance from Lua\n";
            return nullptr;
        }
        uint64_t entity_id = static_cast<uint64_t>(luaL_checkinteger(L, 1));
        flecs::entity entity = world->_world->entity(entity_id);
        LuaChunkEntity* entity_data = entity.get_mut<LuaChunkEntity>();
        if (!entity_data) {
            std::cout << "ChunkEntity does not have LuaChunkEntity component\n";
            return nullptr;
        }
        return entity_data;
    }

    static flecs::entity get_flecs_entity_from_lua(lua_State* L) {
        World* world = get_world_from_lua(L);
        if (!world) {
            std::cout << "Failed to get World instance from Lua\n";
            return flecs::entity::null();
        }
        uint64_t entity_id = static_cast<uint64_t>(luaL_checkinteger(L, 1));
        flecs::entity entity = world->_world->entity(entity_id);
        const LuaChunkEntity* entity_data = entity.get<LuaChunkEntity>();
        if (!entity_data) {
            std::cout << "ChunkEntity does not have LuaChunkEntity component\n";
            return flecs::entity::null();
        }
        return entity;
    }

public:
    World(const char *path = nullptr): _id(uuid::v4::UUID::New()) {
        // Initialize graphics resources
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
        _entity_pipeline = sg_make_pipeline(&desc);
        _tilemap = $Assets.get<Texture>(TILEMAP_PATH);

        // Initialize chunk manager
        $Chunks.initialize(&_camera, _tilemap, _id);

        if (path != nullptr)
            if (!_import(path))
                throw std::runtime_error("Failed to import world from archive");

        // Initialize world directory and print its location
        std::string world_dir = _get_world_directory();
        std::cout << fmt::format("World initialized with UUID: {}\n", _id.String());
        std::cout << fmt::format("Chunk save directory: {}\n", world_dir);

        ecs_os_set_api_defaults();
        ecs_os_api_t os_api = ecs_os_api;
        os_api.abort_ = _abort;
        os_api.log_ = _log;
        ecs_os_set_api(&os_api);
        ecs_log_enable_colors(false);


#pragma region Lua Bindings
        _world = new flecs::world();
        ecs_world_t *w = _world->c_ptr();
        ECS_IMPORT(w, FlecsLua);
        ECS_IMPORT(w, FlecsMeta);
        // Import C++ modules
#define X(MODULE) _world->import<MODULE>();
        MODULES
#undef X
        L = ecs_lua_get_state(_world->c_ptr());
        ecs_assert(L != NULL, ECS_INTERNAL_ERROR, NULL);
        
        // Set Lua state in ChunkManager
        $Chunks.set_lua_state(L);

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

        lua_pushlightuserdata(L, this);
        lua_setfield(L, LUA_REGISTRYINDEX, "__world__");

        lua_pushlightuserdata(L, &_chunk_entities);
        lua_setfield(L, LUA_REGISTRYINDEX, "__chunk_entities__");

        lua_pushlightuserdata(L, &_screen_entities);
        lua_setfield(L, LUA_REGISTRYINDEX, "__screen_entities__");

        lua_register(L, "hide_cursor", [](lua_State* L) -> int {
            sapp_show_mouse(false);
            return 0;
        });

        lua_register(L, "show_cursor", [](lua_State* L) -> int {
            sapp_show_mouse(true);
            return 0;
        });

        lua_register(L, "get_texture", [](lua_State* L) -> int {
            World* world = get_world_from_lua(L);
            if (!world)
                lua_pushnil(L);
            else {
                const char* path = luaL_checkstring(L, 1);
                if (world->has_texture_been_registered(path)) {
                    uint32_t texture_id = world->get_texture_id(path);
                    lua_pushinteger(L, texture_id);
                } else {
                    uint32_t texture_id = world->register_texture(path);
                    lua_pushinteger(L, texture_id);
                }
            }
            return 1;
        });

        lua_register(L, "window_width", [](lua_State* L) -> int {
            lua_pushinteger(L, sapp_width());
            return 1;
        });

        lua_register(L, "window_height", [](lua_State* L) -> int {
            lua_pushinteger(L, sapp_height());
            return 1;
        });

        lua_register(L, "framebuffer_width", [](lua_State* L) -> int {
            lua_pushinteger(L, framebuffer_width());
            return 1;
        });

        lua_register(L, "framebuffer_height", [](lua_State* L) -> int {
            lua_pushinteger(L, framebuffer_height());
            return 1;
        });

        lua_register(L, "frame_duration", [](lua_State* L) -> int {
            lua_pushnumber(L, sapp_frame_duration());
            return 1;
        });

        lua_register(L, "framebuffer_resize", [](lua_State* L) -> int {
            int width = static_cast<int>(luaL_checkinteger(L, 1));
            int height = static_cast<int>(luaL_checkinteger(L, 2));
            framebuffer_resize(width, height);
            return 0; // No return value
        });

        lua_register(L, "chunk_index", [](lua_State* L) -> int {
            int x = static_cast<int>(luaL_checkinteger(L, 1));
            int y = static_cast<int>(luaL_checkinteger(L, 2));
            uint64_t result = index(x, y);
            lua_pushinteger(L, result);
            return 1;
        });

        lua_register(L, "chunk_unindex", [](lua_State* L) -> int {
            uint64_t i = luaL_checkinteger(L, 1);
            auto coords = unindex(i);
            lua_pushinteger(L, coords.first);
            lua_pushinteger(L, coords.second);
            return 2; // Return two values: x, y
        });
        
        lua_register(L, "poisson", [](lua_State* L) -> int {
            World* world = get_world_from_lua(L);
            if (!world)
                return 0;
            int num_args = lua_gettop(L);
            if (num_args < 3) {
                luaL_error(L, "poisson requires at least 3 arguments: chunk_x, chunk_y, radius");
                return 0;
            }

            // Get chunk coordinates (can be either x,y or index)
            long long chunk_x;
            long long chunk_y;
            if (num_args >= 4 && lua_isinteger(L, 1) && lua_isinteger(L, 2) && lua_isnumber(L, 3)) {
                // Format: chunk_x, chunk_y, radius, [k], [invert], [region_x], [region_y], [region_w], [region_h]
                chunk_x = luaL_checkinteger(L, 1);
                chunk_y = luaL_checkinteger(L, 2);
            } else {
                // Format: chunk_index, radius, [k], [invert], [region_x], [region_y], [region_w], [region_h]
                uint64_t chunk_index = luaL_checkinteger(L, 1);
                auto coords = unindex(chunk_index);
                chunk_x = coords.first;
                chunk_y = coords.second;
                // Shift all other arguments by 1
                for (int i = 2; i <= num_args; i++)
                    lua_pushvalue(L, i);
                lua_remove(L, 1); // Remove the index
                num_args--; // Adjust argument count
            }

            float radius = luaL_checknumber(L, (num_args >= 4) ? 3 : 2);
            long long k = 30;
            bool invert = false;
            Rect region = Rect(0, 0, CHUNK_WIDTH, CHUNK_HEIGHT);
            int arg_offset = (num_args >= 4) ? 4 : 3; // Starting position for optional args
            if (num_args > arg_offset)
                k = luaL_checkinteger(L, arg_offset);
            if (num_args > arg_offset + 1)
                invert = lua_toboolean(L, arg_offset + 1);
            if (num_args > arg_offset + 2)
                region.x = static_cast<int>(luaL_checkinteger(L, arg_offset + 2));
            if (num_args > arg_offset + 3)
                region.y = static_cast<int>(luaL_checkinteger(L, arg_offset + 3));
            if (num_args > arg_offset + 4)
                region.w = static_cast<int>(luaL_checkinteger(L, arg_offset + 4));
            if (num_args > arg_offset + 5)
                region.h = static_cast<int>(luaL_checkinteger(L, arg_offset + 5));

            std::vector<glm::vec2> points;
            $Chunks.get_chunk(static_cast<int>(chunk_x), static_cast<int>(chunk_y), [&](Chunk* chunk) {
                if (!chunk || !chunk->is_filled())
                    return; // Will result in empty points vector
                points = chunk->poisson(radius, static_cast<int>(k), invert, true, CHUNK_SIZE / 4, region);
            });
            
            if (points.empty()) {
                lua_pushnil(L);
                return 1; // Return nil if chunk not found or not filled
            }
            lua_createtable(L, static_cast<int>(points.size()), 0);
            for (size_t i = 0; i < points.size(); i++) {
                lua_createtable(L, 2, 0);
                lua_pushnumber(L, points[i].x);
                lua_rawseti(L, -2, 1);
                lua_pushnumber(L, points[i].y);
                lua_rawseti(L, -2, 2);
                lua_rawseti(L, -2, i + 1);
            }
            return 1;
        });

        lua_register(L, "camera_position", [](lua_State *L) -> int {
            World* world = get_world_from_lua(L);
            if (!world) {
                std::cout << "World instance not found in Lua registry in camera_position\n";
                lua_pushnil(L);
                return 1;
            }
            glm::vec2 pos = world->camera()->position();
            lua_newtable(L);
            lua_pushnumber(L, pos.x);
            lua_setfield(L, -2, "x");
            lua_pushnumber(L, pos.y);
            lua_setfield(L, -2, "y");
            return 1;
        });

        lua_register(L, "camera_set_position", [](lua_State *L) -> int {
            float x = 0;
            float y = 0;
            if (lua_istable(L, 1)) {
                lua_getfield(L, 1, "x");
                lua_getfield(L, 1, "y");
                x = static_cast<float>(luaL_checknumber(L, -2));
                y = static_cast<float>(luaL_checknumber(L, -1));
                lua_pop(L, 2); // Remove x and y from stack
            } else {
                x = static_cast<float>(luaL_checknumber(L, 1));
                y = static_cast<float>(luaL_checknumber(L, 2));
            }
            World* world = get_world_from_lua(L);
            if (world)
                world->camera()->set_position(glm::vec2(x, y));
            return 0;
        });

        lua_register(L, "camera_move", [](lua_State *L) -> int {
            float x = 0;
            float y = 0;
            if (lua_istable(L, 1)) {
                lua_getfield(L, 1, "x");
                lua_getfield(L, 1, "y");
                x = static_cast<float>(luaL_checknumber(L, -2));
                y = static_cast<float>(luaL_checknumber(L, -1));
                lua_pop(L, 2); // Remove x and y from stack
            } else {
                x = static_cast<float>(luaL_checknumber(L, 1));
                y = static_cast<float>(luaL_checknumber(L, 2));
            }
            World* world = get_world_from_lua(L);
            if (world)
                world->camera()->move_by(glm::vec2(x, y));
            return 0;
        });

        lua_register(L, "camera_zoom", [](lua_State *L) -> int {
            World* world = get_world_from_lua(L);
            lua_pushnumber(L, world ? world->camera()->zoom() : 0.f);
            return 1;
        });

        lua_register(L, "camera_set_zoom", [](lua_State *L) -> int {
            float z = static_cast<float>(luaL_checknumber(L, 1));
            World *world = get_world_from_lua(L);
            if (world)
                world->camera()->set_zoom(z);
            return 0;
        });

        lua_register(L, "camera_zoom_by", [](lua_State *L) -> int {
            float delta = static_cast<float>(luaL_checknumber(L, 1));
            World *world = get_world_from_lua(L);
            if (world)
                world->camera()->zoom_by(delta);
            return 0;
        });

        lua_register(L, "camera_bounds", [](lua_State *L) -> int {
            World* world = get_world_from_lua(L);
            if (!world) {
                lua_pushnil(L);
                return 1;
            } else {
                Rect bounds = world->camera()->bounds();
                lua_newtable(L);
                lua_pushnumber(L, bounds.x);
                lua_setfield(L, -2, "x");
                lua_pushnumber(L, bounds.y);
                lua_setfield(L, -2, "y");
                lua_pushnumber(L, bounds.w);
                lua_setfield(L, -2, "w");
                lua_pushnumber(L, bounds.h);
                lua_setfield(L, -2, "h");
                return 1;
            }
        });

        lua_register(L, "world_to_screen", [](lua_State *L) -> int {
            World* world = get_world_from_lua(L);
            if (!world) {
                lua_pushnil(L);
                return 1;
            } else {
                float world_x = 0;
                float world_y = 0;
                if (lua_istable(L, 1)) {
                    lua_getfield(L, 1, "x");
                    lua_getfield(L, 1, "y");
                    world_x = static_cast<float>(luaL_checknumber(L, -2));
                    world_y = static_cast<float>(luaL_checknumber(L, -1));
                    lua_pop(L, 2); // Remove x and y from stack
                } else {
                    world_x = static_cast<float>(luaL_checknumber(L, 1));
                    world_y = static_cast<float>(luaL_checknumber(L, 2));
                }
                glm::vec2 screen_pos = world->camera()->world_to_screen(glm::vec2(world_x, world_y));
                lua_newtable(L);
                lua_pushnumber(L, screen_pos.x);
                lua_setfield(L, -2, "x");
                lua_pushnumber(L, screen_pos.y);
                lua_setfield(L, -2, "y");
                return 1;
            }
        });

        lua_register(L, "screen_to_world", [](lua_State *L) -> int {
            World* world = get_world_from_lua(L);
            if (!world) {
                lua_pushnil(L);
                return 0;
            } else {
                float screen_x = 0;
                float screen_y = 0;
                if (lua_istable(L, 1)) {
                    lua_getfield(L, 1, "x");
                    lua_getfield(L, 1, "y");
                    screen_x = static_cast<float>(luaL_checknumber(L, -2));
                    screen_y = static_cast<float>(luaL_checknumber(L, -1));
                    lua_pop(L, 2); // Remove x and y from stack
                } else {
                    screen_x = static_cast<float>(luaL_checknumber(L, 1));
                    screen_y = static_cast<float>(luaL_checknumber(L, 2));
                }
                glm::vec2 world_pos = world->camera()->screen_to_world(glm::vec2(screen_x, screen_y));
                lua_newtable(L);
                lua_pushnumber(L, world_pos.x);
                lua_setfield(L, -2, "x");
                lua_pushnumber(L, world_pos.y);
                lua_setfield(L, -2, "y");
                return 1;
            }
        });

        lua_register(L, "world_to_tile", [](lua_State *L) -> int {
            float world_x = 0;
            float world_y = 0;
            if (lua_istable(L, 1)) {
                lua_getfield(L, 1, "x");
                lua_getfield(L, 1, "y");
                world_x = static_cast<float>(luaL_checknumber(L, -2));
                world_y = static_cast<float>(luaL_checknumber(L, -1));
                lua_pop(L, 2); // Remove x and y from stack
            } else {
                world_x = static_cast<float>(luaL_checknumber(L, 1));
                world_y = static_cast<float>(luaL_checknumber(L, 2));
            }
            glm::vec2 tile = Camera::world_to_tile(glm::vec2(world_x, world_y));
            lua_newtable(L);
            lua_pushnumber(L, tile.x);
            lua_setfield(L, -2, "x");
            lua_pushnumber(L, tile.y);
            lua_setfield(L, -2, "y");
            return 1;
        });

        lua_register(L, "world_to_chunk", [](lua_State *L) -> int {
            float world_x = 0;
            float world_y = 0;
            if (lua_istable(L, 1)) {
                lua_getfield(L, 1, "x");
                lua_getfield(L, 1, "y");
                world_x = static_cast<float>(luaL_checknumber(L, -2));
                world_y = static_cast<float>(luaL_checknumber(L, -1));
                lua_pop(L, 2); // Remove x and y from stack
            } else {
                world_x = static_cast<float>(luaL_checknumber(L, 1));
                world_y = static_cast<float>(luaL_checknumber(L, 2));
            }
            glm::vec2 chunk = Camera::world_to_chunk(glm::vec2(world_x, world_y));
            lua_newtable(L);
            lua_pushnumber(L, chunk.x);
            lua_setfield(L, -2, "x");
            lua_pushnumber(L, chunk.y);
            lua_setfield(L, -2, "y");
            return 1;
        });

        lua_register(L, "chunk_to_world", [](lua_State *L) -> int {
            int cx = 0;
            int cy = 0;
            if (lua_istable(L, 1)) {
                lua_getfield(L, 1, "x");
                lua_getfield(L, 1, "y");
                cx = static_cast<int>(luaL_checkinteger(L, -2));
                cy = static_cast<int>(luaL_checkinteger(L, -1));
                lua_pop(L, 2); // Remove x and y from stack
            } else {
                cx = static_cast<int>(luaL_checkinteger(L, 1));
                cy = static_cast<int>(luaL_checkinteger(L, 2));
            }
            glm::vec2 world_pos = Camera::chunk_to_world(cx, cy);
            lua_newtable(L);
            lua_pushnumber(L, world_pos.x);
            lua_setfield(L, -2, "x");
            lua_pushnumber(L, world_pos.y);
            lua_setfield(L, -2, "y");
            return 1;
        });

        lua_register(L, "tile_to_world", [](lua_State *L) -> int {
            glm::vec2 world_pos;
            int cx = 0;
            int cy = 0;
            int tx = 0;
            int ty = 0;
            if (lua_istable(L, 1)) {
                if (lua_gettop(L) == 1) {
                    lua_getfield(L, 1, "chunk_x");
                    lua_getfield(L, 1, "chunk_y");
                    lua_getfield(L, 1, "tile_x");
                    lua_getfield(L, 1, "tile_y");
                    cx = static_cast<int>(luaL_checkinteger(L, -4));
                    cy = static_cast<int>(luaL_checkinteger(L, -3));
                    tx = static_cast<int>(luaL_checkinteger(L, -2));
                    ty = static_cast<int>(luaL_checkinteger(L, -1));
                } else {
                    lua_getfield(L, 1, "x");
                    lua_getfield(L, 1, "y");
                    lua_getfield(L, 2, "x");
                    lua_getfield(L, 2, "y");
                    cx = static_cast<int>(luaL_checkinteger(L, -4));
                    cy = static_cast<int>(luaL_checkinteger(L, -3));
                    tx = static_cast<int>(luaL_checkinteger(L, -2));
                    ty = static_cast<int>(luaL_checkinteger(L, -1));
                }
                lua_pop(L, 4); // Remove chunk_x, chunk_y, tile_x, tile_y from stack
            } else {
                cx = static_cast<int>(luaL_checkinteger(L, 1));
                cy = static_cast<int>(luaL_checkinteger(L, 2));
                tx = static_cast<int>(luaL_checkinteger(L, 3));
                ty = static_cast<int>(luaL_checkinteger(L, 4));
            }
            world_pos = Camera::tile_to_world(cx, cy, tx, ty);
            lua_newtable(L);
            lua_pushnumber(L, world_pos.x);
            lua_setfield(L, -2, "x");
            lua_pushnumber(L, world_pos.y);
            lua_setfield(L, -2, "y");
            return 1;
        });



        lua_register(L, "set_entity_world_position", [](lua_State *L) -> int {
            flecs::entity e = get_flecs_entity_from_lua(L);
            if (!e.is_valid()) {
                std::cout << "ERROR! Invalid entity in set_entity_world_position\n";
                return 0;
            }
            LuaChunkEntity* entity_data = e.get_mut<LuaChunkEntity>();
            if (!entity_data) {
                std::cout << fmt::format("ERROR! ChunkEntity {} is missing LuaChunkEntity component\n", e.id());
                return 0;
            }
            LuaChunkXY *chunk = e.get_mut<LuaChunkXY>();
            if (!chunk) {
                std::cout << fmt::format("ERROR! ChunkEntity {} is missing LuaChunk component\n", e.id());
                return 0;
            }
            if (lua_istable(L, 2)) {
                lua_getfield(L, 2, "x");
                lua_getfield(L, 2, "y");
                entity_data->x = static_cast<float>(luaL_checknumber(L, -2));
                entity_data->y = static_cast<float>(luaL_checknumber(L, -1));
                lua_pop(L, 2); // Remove x and y from stack
            } else {
                entity_data->x = static_cast<float>(luaL_checknumber(L, 2));
                entity_data->y = static_cast<float>(luaL_checknumber(L, 3));
            }
            glm::vec2 _chunk = Camera::world_to_chunk({entity_data->x, entity_data->y});
            chunk->x = static_cast<int>(_chunk.x);
            chunk->y = static_cast<int>(_chunk.y);
            return 0;
        });

        lua_register(L, "get_entity_world_position", [](lua_State *L) -> int {
            const LuaChunkEntity* entity_data = get_entity_from_lua(L);
            if (!entity_data) {
                std::cout << "ERROR! Invalid entity in get_entity_world_position\n";
                lua_pushnil(L);
                return 1;
            }
            lua_newtable(L);
            lua_pushnumber(L, entity_data->x);
            lua_setfield(L, -2, "x");
            lua_pushnumber(L, entity_data->y);
            lua_setfield(L, -2, "y");
            return 1;
        });

        lua_register(L, "set_entity_position", [](lua_State *L) -> int {
            flecs::entity e = get_flecs_entity_from_lua(L);
            if (!e.is_valid()) {
                std::cout << "ERROR! Invalid entity in set_entity_position\n";
                return 0;
            }
            LuaChunkEntity* entity_data = e.get_mut<LuaChunkEntity>();
            if (!entity_data) {
                std::cout << fmt::format("ERROR! ChunkEntity {} is missing LuaChunkEntity component\n", e.id());
                return 0;
            }
            LuaChunkXY *chunk = e.get_mut<LuaChunkXY>();
            if (!chunk) {
                std::cout << fmt::format("ERROR! ChunkEntity {} is missing LuaChunk component\n", e.id());
                return 0;
            }
            int cx = 0;
            int cy = 0;
            int tx = 0;
            int ty = 0;
            if (lua_istable(L, 2)) {
                if (lua_gettop(L) == 2) {
                    lua_getfield(L, 2, "chunk_x");
                    lua_getfield(L, 2, "chunk_y");
                    lua_getfield(L, 2, "tile_x");
                    lua_getfield(L, 2, "tile_y");
                    cx = static_cast<int>(luaL_checkinteger(L, -4));
                    cy = static_cast<int>(luaL_checkinteger(L, -3));
                    tx = static_cast<int>(luaL_checkinteger(L, -2));
                    ty = static_cast<int>(luaL_checkinteger(L, -1));
                } else if (lua_gettop(L) == 3) {
                    lua_getfield(L, 2, "x");
                    lua_getfield(L, 2, "y");
                    lua_getfield(L, 3, "x");
                    lua_getfield(L, 3, "y");
                    cx = static_cast<int>(luaL_checkinteger(L, -4));
                    cy = static_cast<int>(luaL_checkinteger(L, -3));
                    tx = static_cast<int>(luaL_checkinteger(L, -2));
                    ty = static_cast<int>(luaL_checkinteger(L, -1));
                }
                lua_pop(L, 4); // Remove values from stack
            } else {
                cx = static_cast<int>(luaL_checkinteger(L, 2));
                cy = static_cast<int>(luaL_checkinteger(L, 3));
                tx = static_cast<int>(luaL_checkinteger(L, 4));
                ty = static_cast<int>(luaL_checkinteger(L, 5));
            }
            glm::vec2 world_pos = Camera::tile_to_world(cx, cy, tx, ty);
            entity_data->x = world_pos.x;
            entity_data->y = world_pos.y;
            chunk->x = cx;
            chunk->y = cy;
            return 0;
        });

        lua_register(L, "get_entity_position", [](lua_State *L) -> int {
            const LuaChunkEntity* entity_data = get_entity_from_lua(L);
            if (!entity_data) {
                std::cout << "ERROR! Invalid entity in get_entity_position\n";
                lua_pushnil(L);
                return 1;
            }
            glm::vec2 chunk = Camera::world_to_chunk({entity_data->x, entity_data->y});
            glm::vec2 tile = Camera::world_to_tile({entity_data->x, entity_data->y});
            lua_newtable(L);
            lua_pushinteger(L, static_cast<int>(chunk.x));
            lua_setfield(L, -2, "chunk_x");
            lua_pushinteger(L, static_cast<int>(chunk.y));
            lua_setfield(L, -2, "chunk_y");
            lua_pushinteger(L, static_cast<int>(tile.x));
            lua_setfield(L, -2, "tile_x");
            lua_pushinteger(L, static_cast<int>(tile.y));
            lua_setfield(L, -2, "tile_y");
            return 1;
        });

        lua_register(L, "set_entity_size", [](lua_State *L) -> int {
            LuaChunkEntity* entity_data = get_mutable_entity_from_lua(L);
            switch (lua_gettop(L)) {
                case 2:
                    if (lua_istable(L, 2)) {
                        lua_getfield(L, 2, "width");
                        lua_getfield(L, 2, "height");
                        entity_data->width = static_cast<float>(luaL_checknumber(L, -2));
                        entity_data->height = static_cast<float>(luaL_checknumber(L, -1));
                        lua_pop(L, 2); // Remove width and height from stack
                    } else
                        entity_data->width = entity_data->height = static_cast<float>(luaL_checknumber(L, 2));
                    break;
                case 3:
                    entity_data->width = static_cast<float>(luaL_checknumber(L, 2));
                    entity_data->height = static_cast<float>(luaL_checknumber(L, 3));
                    break;
                default:
                    std::cout << "ERROR! set_entity_size expects either (entity, size) or (entity, width, height)\n";
            }
            return 0;
        });

        lua_register(L, "get_entity_size", [](lua_State *L) -> int {
            const LuaChunkEntity* entity_data = get_entity_from_lua(L);
            if (!entity_data) {
                std::cout << "ERROR! Invalid entity in get_entity_size\n";
                lua_pushnil(L);
                return 1;
            }
            lua_newtable(L);
            lua_pushnumber(L, entity_data->width);
            lua_setfield(L, -2, "width");
            lua_pushnumber(L, entity_data->height);
            lua_setfield(L, -2, "height");
            return 1;
        });

        lua_register(L, "set_entity_z", [](lua_State *L) -> int {
            LuaChunkEntity* entity_data = get_mutable_entity_from_lua(L);
            if (!entity_data)
                std::cout << "ERROR! Invalid entity in set_entity_z\n";
            else
                entity_data->z_index = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        });

        lua_register(L, "get_entity_z", [](lua_State *L) -> int {
            const LuaChunkEntity* entity_data = get_entity_from_lua(L);
            if (!entity_data) {
                std::cout << "ERROR! Invalid entity in get_entity_z\n";
                lua_pushnil(L);
            } else
                lua_pushnumber(L, entity_data->z_index);
            return 1;
        });

        lua_register(L, "set_entity_rotation", [](lua_State *L) -> int {
            LuaChunkEntity* entity_data = get_mutable_entity_from_lua(L);
            if (!entity_data)
                std::cout << "ERROR! Invalid entity in set_entity_rotation\n";
            else
                entity_data->rotation = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        });

        lua_register(L, "get_entity_rotation", [](lua_State *L) -> int {
            const LuaChunkEntity* entity_data = get_entity_from_lua(L);
            if (!entity_data) {
                std::cout << "ERROR! Invalid entity in get_entity_rotation\n";
                lua_pushnil(L);
            } else
                lua_pushnumber(L, entity_data->rotation);
            return 1;
        });

        lua_register(L, "set_entity_scale", [](lua_State *L) -> int {
            LuaChunkEntity* entity_data = get_mutable_entity_from_lua(L);
            if (!entity_data) {
                std::cout << "ERROR! Invalid entity in set_entity_scale\n";
                return 0;
            }
            switch (lua_gettop(L)) {
                case 2:
                    if (lua_istable(L, 2)) {
                        lua_getfield(L, 2, "x");
                        lua_getfield(L, 2, "y");
                        entity_data->scale_x = static_cast<float>(luaL_checknumber(L, -2));
                        entity_data->scale_y = static_cast<float>(luaL_checknumber(L, -1));
                        lua_pop(L, 2); // Remove x and y from stack
                    } else
                        entity_data->scale_x = entity_data->scale_y = static_cast<float>(luaL_checknumber(L, 2));
                    break;
                case 3:
                    entity_data->scale_x = static_cast<float>(luaL_checknumber(L, 2));
                    entity_data->scale_y = static_cast<float>(luaL_checknumber(L, 3));
                    break;
                default:
                    std::cout << "ERROR! set_entity_scale expects either (entity, scale) or (entity, scale_x, scale_y)\n";
            }
            return 0;
        });

        lua_register(L, "get_entity_scale", [](lua_State *L) -> int {
            const LuaChunkEntity* entity_data = get_entity_from_lua(L);
            if (!entity_data) {
                std::cout << "ERROR! Invalid entity in get_entity_scale\n";
                lua_pushnil(L);
            } else {
                lua_newtable(L);
                lua_pushnumber(L, entity_data->scale_x);
                lua_setfield(L, -2, "x");
                lua_pushnumber(L, entity_data->scale_y);
                lua_setfield(L, -2, "y");
            }
            return 1;
        });

        lua_register(L, "set_entity_clip", [](lua_State *L) -> int {
            LuaChunkEntity* entity_data = get_mutable_entity_from_lua(L);
            if (!entity_data) {
                std::cout << "ERROR! Invalid entity in set_entity_clip\n";
                return 0;
            }
            switch (lua_gettop(L)) {
                case 1:
                    entity_data->clip_x = 0;
                    entity_data->clip_y = 0;
                    entity_data->clip_width = 0;
                    entity_data->clip_height = 0;
                    break;
                case 2:
                    lua_getfield(L, 2, "x");
                    lua_getfield(L, 2, "y");
                    lua_getfield(L, 2, "width");
                    lua_getfield(L, 2, "height");
                    entity_data->clip_x = static_cast<int>(luaL_checkinteger(L, -4));
                    entity_data->clip_y = static_cast<int>(luaL_checkinteger(L, -3));
                    entity_data->clip_width = static_cast<int>(luaL_checkinteger(L, -2));
                    entity_data->clip_height = static_cast<int>(luaL_checkinteger(L, -1));
                    lua_pop(L, 4); // Remove x, y, width, height from stack
                    break;
                case 3:
                    lua_getfield(L, 2, "x");
                    lua_getfield(L, 2, "y");
                    lua_getfield(L, 3, "width");
                    lua_getfield(L, 3, "height");
                    entity_data->clip_x = static_cast<int>(luaL_checkinteger(L, -4));
                    entity_data->clip_y = static_cast<int>(luaL_checkinteger(L, -3));
                    entity_data->clip_width = static_cast<int>(luaL_checkinteger(L, -2));
                    entity_data->clip_height = static_cast<int>(luaL_checkinteger(L, -1));
                    lua_pop(L, 4); // Remove x, y, width, height from stack
                    break;
                case 5:
                    entity_data->clip_width = static_cast<int>(luaL_checkinteger(L, 2));
                    entity_data->clip_height = static_cast<int>(luaL_checkinteger(L, 3));
                    entity_data->clip_x = static_cast<int>(luaL_checkinteger(L, 4));
                    entity_data->clip_y = static_cast<int>(luaL_checkinteger(L, 5));
                    break;
                default:
                    std::cout << "ERROR! set_entity_clip expects either (entity) to clear clipping, (entity, {x=.., y=.., width=.., height=..}) or (entity, width, height, x, y)\n";
            }
            return 0;
        });

        lua_register(L, "set_entity_clip_size", [](lua_State *L) -> int {
            LuaChunkEntity* entity_data = get_mutable_entity_from_lua(L);
            if (!entity_data) {
                std::cout << "ERROR! Invalid entity in set_entity_clip_size\n";
                return 0;
            }
            if (lua_istable(L, 2)) {
                lua_getfield(L, 2, "width");
                lua_getfield(L, 2, "height");
                entity_data->clip_width = static_cast<int>(luaL_checkinteger(L, -2));
                entity_data->clip_height = static_cast<int>(luaL_checkinteger(L, -1));
                lua_pop(L, 2); // Remove width and height from stack
            } else {
                entity_data->clip_width = static_cast<int>(luaL_checkinteger(L, 2));
                entity_data->clip_height = static_cast<int>(luaL_checkinteger(L, 3));
            }
            return 0;
        });

        lua_register(L, "set_entity_clip_offset", [](lua_State *L) -> int {
            LuaChunkEntity* entity_data = get_mutable_entity_from_lua(L);
            if (!entity_data) {
                std::cout << "ERROR! Invalid entity in set_entity_clip_offset\n";
                return 0;
            }
            entity_data->clip_x = static_cast<int>(luaL_checkinteger(L, 2));
            entity_data->clip_y = static_cast<int>(luaL_checkinteger(L, 3));
            return 0;
        });

        lua_register(L, "get_entity_clip", [](lua_State *L) -> int {
            const LuaChunkEntity* entity_data = get_entity_from_lua(L);
            if (!entity_data) {
                std::cout << "ERROR! Invalid entity in get_entity_clip\n";
                lua_pushnil(L);
                return 1;
            }
            lua_newtable(L);
            lua_pushinteger(L, entity_data->clip_x);
            lua_setfield(L, -2, "x");
            lua_pushinteger(L, entity_data->clip_y);
            lua_setfield(L, -2, "y");
            lua_pushinteger(L, entity_data->clip_width);
            lua_setfield(L, -2, "width");
            lua_pushinteger(L, entity_data->clip_height);
            lua_setfield(L, -2, "height");
            return 1;
        });

        lua_register(L, "set_entity_speed", [](lua_State *L) -> int {
            LuaChunkEntity* entity_data = get_mutable_entity_from_lua(L);
            if (!entity_data) {
                std::cout << "ERROR! Invalid entity in set_entity_speed\n";
                return 0;
            }
            entity_data->speed = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        });

        lua_register(L, "get_entity_speed", [](lua_State *L) -> int {
            const LuaChunkEntity* entity_data = get_entity_from_lua(L);
            if (!entity_data) {
                std::cout << "ERROR! Invalid entity in get_entity_speed\n";
                lua_pushnil(L);
                return 1;
            }
            lua_pushnumber(L, entity_data->speed);
            return 1;
        });

        lua_register(L, "set_entity_target", [](lua_State *L) -> int {
            flecs::entity entity = get_flecs_entity_from_lua(L);
            int x = static_cast<int>(luaL_checkinteger(L, 2));
            int y = static_cast<int>(luaL_checkinteger(L, 3));
            if (x < 0 || x >= CHUNK_WIDTH)
                x = std::clamp(x, 0, CHUNK_WIDTH - 1);
            if (y < 0 || y >= CHUNK_HEIGHT)
                y = std::clamp(y, 0, CHUNK_HEIGHT - 1);
            if (!entity.is_valid()) {
                std::cout << "ERROR! Invalid entity in set_entity_target\n";
                return 0;
            }
            LuaChunkXY *lchunk = entity.get_mut<LuaChunkXY>();
            if (!lchunk) {
                std::cout << fmt::format("ERROR! ChunkEntity {} is missing LuaChunk component\n", entity.id());
                return 0;
            }
            World* world = get_world_from_lua(L);
            if (!world) {
                std::cout << "ERROR! World instance not found in Lua registry in set_entity_target\n";
                return 0;
            }
            if (!$Chunks.is_chunk_loaded(lchunk->x, lchunk->y)) {
                std::cout << fmt::format("ERROR! Cannot set target for entity {} because its chunk ({},{}) is not loaded\n", entity.id(), lchunk->x, lchunk->y);
                return 0;
            }
            entity.set<LuaTarget>({x, y});
            return 0;
        });

        lua_register(L, "get_entity_target", [](lua_State *L) -> int {
            flecs::entity entity = get_flecs_entity_from_lua(L);
            const LuaTarget *target = entity.get<LuaTarget>();
            if (target) {
                lua_pushnumber(L, target->x);
                lua_pushnumber(L, target->y);
                return 2;
            } else {
                lua_pushnil(L);
                lua_pushnil(L);
                return 2;
            }
        });

        lua_register(L, "clear_entity_target", [](lua_State *L) -> int {
            flecs::entity entity = get_flecs_entity_from_lua(L);
            entity.remove<LuaTarget>();
            return 0;
        });

        lua_register(L, "entity_has_target", [](lua_State *L) -> int {
            flecs::entity entity = get_flecs_entity_from_lua(L);
            if (!entity.is_valid()) {
                std::cout << "ERROR! Invalid entity in entity_has_target\n";
                lua_pushboolean(L, false);
                return 1;
            }
            lua_pushboolean(L, entity.has<LuaTarget>());
            return 1;
        });

        lua_register(L, "get_entity_bounds", [](lua_State *L) -> int {
            const LuaChunkEntity* entity_data = get_entity_from_lua(L);
            if (!entity_data) {
                std::cout << "ERROR! Invalid entity in get_entity_bounds\n";
                lua_pushnil(L);
                return 1;
            }
            Rect bounds = EntityFactory<LuaChunkEntity>::entity_bounds(*entity_data);
            lua_newtable(L);
            lua_pushinteger(L, bounds.x);
            lua_setfield(L, -2, "x");
            lua_pushinteger(L, bounds.y);
            lua_setfield(L, -2, "y");
            lua_pushinteger(L, bounds.w);
            lua_setfield(L, -2, "width");
            lua_pushinteger(L, bounds.h);
            lua_setfield(L, -2, "height");
            return 1;
        });

        lua_register(L, "is_entity_visible", [](lua_State *L) -> int {
            const LuaChunkEntity* entity_data = get_entity_from_lua(L);
            if (!entity_data) {
                std::cout << "ERROR! Invalid entity in is_entity_visible\n";
                lua_pushboolean(L, false);
                return 1;
            }
            World* world = get_world_from_lua(L);
            if (!world) {
                std::cout << "ERROR! World instance not found in Lua registry in is_entity_visible\n";
                lua_pushboolean(L, false);
                return 1;
            }
            Rect entity_bounds = EntityFactory<LuaChunkEntity>::entity_bounds(*entity_data);
            Rect camera_bounds = world->camera()->bounds();
            lua_pushboolean(L, camera_bounds.intersects(entity_bounds));
            return 1;
        });

        // Expose ChunkEvent types to Lua
        lua_newtable(L);
        lua_pushinteger(L, static_cast<int>(ChunkEvent::Created));
        lua_setfield(L, -2, "created");
        lua_pushinteger(L, static_cast<int>(ChunkEvent::Deleted));
        lua_setfield(L, -2, "deleted");
        lua_pushinteger(L, static_cast<int>(ChunkEvent::VisibilityChanged));
        lua_setfield(L, -2, "visibility_changed");
        lua_setglobal(L, "ChunkEventType");

        // Expose ChunkVisibility types to Lua
        lua_newtable(L);
        lua_pushinteger(L, static_cast<int>(ChunkVisibility::OutOfSign));
        lua_setfield(L, -2, "out_of_sign");
        lua_pushinteger(L, static_cast<int>(ChunkVisibility::Visible));
        lua_setfield(L, -2, "visible");
        lua_pushinteger(L, static_cast<int>(ChunkVisibility::Occluded));
        lua_setfield(L, -2, "occluded");
        lua_setglobal(L, "ChunkVisibility");

        // Register chunk callback management functions
        lua_register(L, "register_chunk_callback", [](lua_State *L) -> int {
            if (!lua_isinteger(L, 1) || !lua_isfunction(L, 2)) {
                luaL_error(L, "register_chunk_callback expects (integer, function)");
                return 0;
            }
            
            int event_type = static_cast<int>(luaL_checkinteger(L, 1));

            // Store the function in the registry and get a reference
            lua_pushvalue(L, 2); // Duplicate the function
            int ref = luaL_ref(L, LUA_REGISTRYINDEX);
            
            $Chunks.register_lua_callback(event_type, ref);
            return 0;
        });

        lua_register(L, "unregister_chunk_callback", [](lua_State *L) -> int {
            if (!lua_isinteger(L, 1)) {
                luaL_error(L, "unregister_chunk_callback expects (integer)");
                return 0;
            }
            
            int event_type = static_cast<int>(luaL_checkinteger(L, 1));
            $Chunks.unregister_lua_callback(event_type);
            return 0;
        });

        lua_register(L, "random_empty_tile_in_chunk", [](lua_State *L) -> int {
            int cx = 0;
            int cy = 0;
            if (lua_istable(L, 1)) {
                lua_getfield(L, 1, "x");
                lua_getfield(L, 1, "y");
                cx = static_cast<int>(luaL_checkinteger(L, -2));
                cy = static_cast<int>(luaL_checkinteger(L, -1));
                lua_pop(L, 2); // Remove x and y from stack
            } else {
                cx = static_cast<int>(luaL_checkinteger(L, 1));
                cy = static_cast<int>(luaL_checkinteger(L, 2));
            }
            World* world = get_world_from_lua(L);
            if (!world) {
                std::cout << "ERROR! World instance not found in Lua registry in random_empty_tile_in_chunk\n";
                lua_pushnil(L);
                return 1;
            }
            auto p = $Chunks.random_walkable_tile_in_chunk(cx, cy);
            if (!p.has_value()) {
                lua_pushnil(L);
                return 1;
            }
            lua_newtable(L);
            lua_pushinteger(L, p->first);
            lua_setfield(L, -2, "x");
            lua_pushinteger(L, p->second);
            lua_setfield(L, -2, "y");
            return 1;
        });

        // Register input functions
        $Input.load_into_lua(L);
#pragma endregion

        if (luaL_loadbuffer(L, reinterpret_cast<const char*>(setup_lua), setup_lua_size, "setup.lua") != LUA_OK ||
            lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK) {
            const char* error_msg = lua_tostring(L, -1);
            fprintf(stderr, "Lua error in setup.lua: %s\n", error_msg);
            lua_pop(L, 1); // Remove error message from stack
            throw std::runtime_error("Internal Error: Failed to execute `setup.lua`");
        }

        GenericAsset *main_lua = $Assets.get<>("main.lua");
        if (main_lua && main_lua->is_valid() && !main_lua->data().empty()) {
            if (luaL_loadbuffer(L, reinterpret_cast<const char*>(main_lua->raw_data()), main_lua->size(), "main.lua") != LUA_OK ||
                lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK) {
                const char* error_msg = lua_tostring(L, -1);
                std::cout << fmt::format("Lua error in main.lua: {}\n", error_msg);
                lua_pop(L, 1); // Remove error message from stack
                throw std::runtime_error("Failed to execute `main.lua`");
            }
        } else
            std::cout << fmt::format("Warning: main.lua not found or invalid, skipping execution\n");
        ecs_assert(L != NULL, ECS_INTERNAL_ERROR, NULL);
    }

    ~World() {
        // Clean up InputManager callbacks before cleaning up chunk callbacks
        $Input.cleanup_lua_callbacks();
        _texture_registry.clear();
        $Chunks.clear();
        if (_world)
            delete _world;
        if (sg_query_shader_state(_shader) == SG_RESOURCESTATE_VALID)
            sg_destroy_shader(_shader);
        if (sg_query_pipeline_state(_pipeline) == SG_RESOURCESTATE_VALID)
            sg_destroy_pipeline(_pipeline);
        if (sg_query_pipeline_state(_entity_pipeline) == SG_RESOURCESTATE_VALID)
            sg_destroy_pipeline(_entity_pipeline);
        _export();
    }

    bool update(float dt) {
        Rect camera_bounds = _camera.bounds();
        Rect max_bounds = _camera.max_bounds();
        
        $Chunks.update_chunks(camera_bounds, max_bounds);
        $Chunks.update_deletion_queue();
        $Chunks.scan_for_chunks(camera_bounds, max_bounds);
        auto events_to_queue = $Chunks.release_chunks();
        $Chunks.queue_events(std::move(events_to_queue));
        $Chunks.fire_chunk_events();

        _chunk_entities.finalize(&_texture_registry, &_camera);
        _screen_entities.finalize(&_texture_registry);
        $Chunks.draw_chunks(_pipeline, _camera.is_dirty());
        sg_apply_pipeline(_entity_pipeline);
        _chunk_entities.flush(&_camera);
        _screen_entities.flush();
        
        return _world->progress(dt);
    }

    Camera* camera() { return &_camera; }
    ChunkEntityFactory& chunk_entities() { return _chunk_entities; }
    ScreenEntityFactory& screen_entities() { return _screen_entities; }

    uint32_t register_texture(const std::string& key) {
        return _texture_registry.reigster_asset(key, $Assets.get<Texture>(key));
    }

    Texture* get_texture_by_id(uint32_t id) {
        return _texture_registry.get_asset(id);
    }

    uint32_t get_texture_id(const std::string& key) {
        return _texture_registry.get_asset_id(key);
    }

    bool has_texture_been_registered(const std::string& key) {
        return _texture_registry.has_asset(key);
    }
};
