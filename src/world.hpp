//
//  world.hpp
//  ice
//
//  Created by George Watson on 28/08/2025.
//

#pragma once

#include "ice.hpp"
#include "flecs.h"
#include "flecs_lua.h"
#include "chunk_manager.hpp"
#include "renderable.hpp"

class World {
    flecs::world *world = nullptr;
    lua_State *L = nullptr;
    ChunkManager *_manager = nullptr;

    static void _abort(void) {
        fprintf(stderr, "ECS: ecs_os_abort() was called!\n");
        fflush(stdout);
        printf("\n");
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

public:
    World(ChunkManager *manager, const char *path): _manager(manager) {
        ecs_os_set_api_defaults();
        ecs_os_api_t os_api = ecs_os_api;
        os_api.abort_ = _abort;
        os_api.log_ = _log;
        ecs_os_set_api(&os_api);
        ecs_log_enable_colors(false);

        world = new flecs::world();
        // FlecsLua still uses C API for import
        ECS_IMPORT(world->c_ptr(), FlecsLua);
        
        // Import C++ modules
#define X(MODULE) world->import<MODULE>();
        MODULES
#undef X
        L = ecs_lua_get_state(world->c_ptr());
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

        int result = luaL_dofile(L, path);
        if (result != LUA_OK) {
            const char* error_msg = lua_tostring(L, -1);
            fprintf(stderr, "Lua error loading %s: %s\n", path, error_msg);
            lua_pop(L, 1);
        }
        ecs_assert(L != NULL, ECS_INTERNAL_ERROR, NULL);
    }

    ~World() {
        if (world)
            delete world;
    }

    bool update(float dt) {
        _manager->update_chunks();
        _manager->scan_for_chunks();
        _manager->release_chunks();
        _manager->draw_chunks();
        return world->progress(dt);
    }

    operator flecs::world*(void) {
        return world;
    }
};
