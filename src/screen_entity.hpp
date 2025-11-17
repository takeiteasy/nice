//
//  screen_entity.hpp
//  nice
//
//  Created by George Watson on 13/09/2025.
//

#pragma once

#include "minilua.h"
#include "flecs.h"
#include "flecs_lua.h"
#include "entity_factory.hpp"
#include <iostream>

ECS_STRUCT(LuaScreenEntity, {
    float x;
    float y;
    float width;
    float height;
    uint32_t texture_id;
    uint32_t z_index;
    float rotation;
    float scale_x;
    float scale_y;
    uint32_t clip_x;
    uint32_t clip_y;
    uint32_t clip_width;
    uint32_t clip_height;
});

class ScreenEntityFactory: public EntityFactory<LuaScreenEntity> {
public:
};

struct ScreenEntity {
    static ScreenEntityFactory* get_screen_entity_factory_world(ecs_world_t *world) {
        lua_State *L = ecs_lua_get_state(world);
        lua_getfield(L, LUA_REGISTRYINDEX, "__screen_entities__");
        ScreenEntityFactory* chunk_entities = static_cast<ScreenEntityFactory*>(lua_touserdata(L, -1));
        lua_pop(L, 1); // Remove the userdata from stack
        if (!chunk_entities) {
            std::cout << "ScreenEntityFactory instance not found in Lua registry\n";
            return nullptr;
        }
        return chunk_entities;
    }

    static ScreenEntityFactory* get_screen_entity_factory(flecs::entity entity) {
        return get_screen_entity_factory_world(entity.world().c_ptr());
    }

    ScreenEntity(flecs::world& world) {
        world.module<ScreenEntity>();
        ecs_world_t *w = world.c_ptr();
        ecs_entity_t scope = ecs_set_scope(w, 0);
        ECS_META_COMPONENT(w, LuaScreenEntity);
        ecs_set_scope(w, scope);

        world.observer<LuaScreenEntity>()
            .event(flecs::OnAdd)
            .each([](flecs::entity entity, LuaScreenEntity& entity_data) {
                // Set default values if they haven't been set
                if (entity_data.scale_x == 0.0f)
                    entity_data.scale_x = 1.0f;
                if (entity_data.scale_y == 0.0f)
                    entity_data.scale_y = 1.0f;
                ScreenEntityFactory *screen_entities = get_screen_entity_factory(entity);
                screen_entities->add_entity(entity);
            });

        auto run_lock = [](flecs::iter_t *it) {
            ScreenEntityFactory *screen_entities = get_screen_entity_factory_world(it->world);
            std::lock_guard<std::shared_mutex> lock(screen_entities->get_lock());
            while (ecs_iter_next(it))
                it->callback(it);
        };

        world.observer<LuaScreenEntity>()
            .event(flecs::OnSet)
            .run(run_lock)
            .each([](flecs::entity entity, LuaScreenEntity& entity_data) {
                ScreenEntityFactory *screen_entities = get_screen_entity_factory(entity);
                screen_entities->update_entity(entity, entity_data, false);
            });

        world.observer<LuaScreenEntity>()
            .event(flecs::OnRemove)
            .run(run_lock)
            .each([](flecs::entity entity, LuaScreenEntity& entity_data) {
                ScreenEntityFactory *screen_entities = get_screen_entity_factory(entity);
                screen_entities->remove_entity(entity, false); // lock=false since run_lock already holds the lock
            });
    }
};
