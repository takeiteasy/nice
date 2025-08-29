//
//  renderable_manager.hpp
//  ice
//
//  Created by George Watson on 29/08/2025.
//

#pragma once

#include "ice.hpp"
#include "global.hpp"
#include "components.hpp"

#define $Renderables RenderableManager::instance()

struct CachedEntity {
    uint64_t chunk_idx;
    uint32_t z_index;
    uint32_t texture_id;
};

class RenderableManager: public Global<RenderableManager> {
    using EntityMap = std::unordered_map<uint32_t, std::unordered_map<uint64_t, std::unordered_map<uint32_t, flecs::entity>>>;
    using EntityMapCache = std::unordered_map<flecs::entity, CachedEntity>;

    flecs::world *_world = nullptr;
    EntityMap _entities;
    mutable std::mutex _entities_lock;
    EntityMapCache _entity_cache;

public:
    void init(flecs::world *world) {
        _world = world;
    }

    void add_entity(flecs::entity entity) {
        std::lock_guard<std::mutex> lock(_entities_lock);
        LuaChunk *chunk = entity.get_mut<LuaChunk>();
        LuaRenderable *renderable = entity.get_mut<LuaRenderable>();
        if (!chunk || !renderable)
            throw std::runtime_error("Entity must have both LuaChunk and LuaRenderable components");
        uint64_t chunk_idx = index(chunk->x, chunk->y);
        _entities[chunk_idx][renderable->z_index][renderable->texture_id] = entity;
        _entity_cache[entity] = {chunk_idx, renderable->z_index, renderable->texture_id};
    }

    void remove_entity(flecs::entity entity) {
        std::lock_guard<std::mutex> lock(_entities_lock);
        LuaChunk *chunk = entity.get_mut<LuaChunk>();
        LuaRenderable *renderable = entity.get_mut<LuaRenderable>();
    }

    void update_entity(flecs::entity entity) {
        std::lock_guard<std::mutex> lock(_entities_lock);
        LuaChunk *chunk = entity.get_mut<LuaChunk>();
        LuaRenderable *renderable = entity.get_mut<LuaRenderable>();
    }

    void finalise() {
        std::lock_guard<std::mutex> lock(_entities_lock);
    }

    void flush() {
        std::lock_guard<std::mutex> lock(_entities_lock);
    }

    void clear() {
        std::lock_guard<std::mutex> lock(_entities_lock);
    }
};

struct Renderable {
    Renderable(flecs::world &world) {
        world.module<Renderable>();

        ecs_world_t *w = world.c_ptr();
        ECS_IMPORT(w, FlecsMeta);

        ecs_entity_t scope = ecs_set_scope(w, 0);

        ECS_META_COMPONENT(w, LuaChunk);
        LuaChunk chunk = {.x = 0, .y = 0};
        ecs_set_ptr(w, 0, LuaChunk, &chunk);
        ECS_META_COMPONENT(w, LuaRenderable);
        LuaRenderable renderable = {
            .x = 0.f,
            .y = 0.f,
            .width = 0.f,
            .height = 0.f,
            .texture_id = 0,
            .z_index = 0,
            .rotation = 0.f,
            .scale_x = 1.f,
            .scale_y = 1.f
        };
        ecs_set_ptr(w, 0, LuaRenderable, &renderable);

        ecs_set_scope(w, scope);

        // Set up observers to automatically manage renderable entities
        world.observer<LuaRenderable>()
            .event(flecs::OnAdd)
            .each([](flecs::entity entity, LuaRenderable& renderable) {
                printf("Renderable added: Entity ID %llu\nX: %.2f, Y: %.2f, Width: %.2f, Height: %.2f, Texture ID: %u, Z-Index: %u, Rotation: %.2f, ScaleX: %.2f, ScaleY: %.2f\n",
                       entity.id(),
                       renderable.x, renderable.y,
                       renderable.width, renderable.height,
                       renderable.texture_id,
                       renderable.z_index,
                       renderable.rotation,
                       renderable.scale_x, renderable.scale_y);
                glm::vec2 chunk = Camera::world_to_chunk(glm::vec2(renderable.x, renderable.y));
                entity.set<LuaChunk>({static_cast<uint32_t>(chunk.x), static_cast<uint32_t>(chunk.y)});
                $Renderables.add_entity(entity);
            });

        world.observer<LuaRenderable>()
            .event(flecs::OnRemove)
            .each([](flecs::entity entity, LuaRenderable& renderable) {
                printf("Renderable removed: Entity ID %llu\n", entity.id());
                $Renderables.remove_entity(entity);
            });


        world.observer<LuaChunk, LuaRenderable>()
            .event(flecs::OnSet)
            .each([](flecs::entity entity, LuaChunk& chunk, LuaRenderable& renderable) {
                $Renderables.update_entity(entity);
            });
    }
};
