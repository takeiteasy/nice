//
//  entity.hpp
//  nice
//
//  Created by George Watson on 06/09/2025.
//

#pragma once

#include "entity_manager.hpp"

struct Entity {
    Entity(flecs::world &world) {
        world.module<Entity>();
        ecs_world_t *w = world.c_ptr();
        ECS_IMPORT(w, FlecsMeta);
        ecs_entity_t scope = ecs_set_scope(w, 0);
        ECS_META_COMPONENT(w, LuaEntity);
        ecs_set_scope(w, scope);

        world.component<LuaChunk>();
        world.component<LuaTarget>();
        world.component<LuaWaypoint>();

        // Set up observers to automatically manage entity_data entities
        world.observer<LuaEntity>()
            .event(flecs::OnAdd)
            .each([](flecs::entity entity, LuaEntity& entity_data) {
                // Set default values if they haven't been set
                if (entity_data.scale_x == 0.0f)
                    entity_data.scale_x = 1.0f;
                if (entity_data.scale_y == 0.0f)
                    entity_data.scale_y = 1.0f;
                if (entity_data.speed == 0.0f)
                    entity_data.speed = 100.0f;
                glm::vec2 chunk = Camera::world_to_chunk(glm::vec2(entity_data.x, entity_data.y));
                entity.set<LuaChunk>({static_cast<uint32_t>(chunk.x), static_cast<uint32_t>(chunk.y)});
                $Entities.add_entity(entity);
            });

        world.observer<LuaEntity, LuaChunk>()
            .event(flecs::OnSet)
            .each([](flecs::entity entity, LuaEntity& entity_data, LuaChunk& chunk) {
                std::lock_guard<std::shared_mutex> lock($Entities.entities_lock());
                glm::vec2 chunk_pos = Camera::world_to_chunk(glm::vec2(entity_data.x, entity_data.y));
                if (chunk.x != static_cast<uint32_t>(chunk_pos.x) ||
                    chunk.y != static_cast<uint32_t>(chunk_pos.y)) {
                    chunk.x  = static_cast<uint32_t>(chunk_pos.x);
                    chunk.y  = static_cast<uint32_t>(chunk_pos.y);
                    entity.set(chunk); // Update the component
                }
            });

#define RUN_LOCK(LOCK) \
        .run([](flecs::iter_t *it) { \
            std::lock_guard<std::shared_mutex> lock(LOCK); \
            while (ecs_iter_next(it)) \
                it->callback(it); \
        })

        world.observer<LuaEntity>()
            .event(flecs::OnRemove)
            RUN_LOCK($Entities.entities_lock())
            .each([](flecs::entity entity, LuaEntity& entity_data) {
                $Entities.remove_entity(entity, false);
            });

        world.observer<LuaEntity, LuaChunk>()
            .event(flecs::OnSet)
            RUN_LOCK($Entities.entities_lock())
            .each([](flecs::entity entity, LuaEntity& entity_data, LuaChunk& chunk) {
                $Entities.update_entity(entity, entity_data, chunk, false);;
            });

        world.observer<LuaChunk, LuaTarget>()
            .event(flecs::OnSet)
            RUN_LOCK($Entities.entities_lock())
            .each([](flecs::entity entity, LuaChunk& chunk, LuaTarget& target) {
                std::cout << fmt::format("Entity {} set target to ({}, {})\n", entity.id(), target.x, target.y);
                $Entities.clear_target(entity);
                $Entities.add_entity_target(entity, chunk, target);
            });
        
        // System to assign next waypoint when entity doesn't have one but has a target
        world.system<LuaTarget>("AssignWaypoint")
            .without<LuaWaypoint>()
            RUN_LOCK($Entities.entities_lock())
            .each([](flecs::entity entity, LuaTarget& target) {
                auto waypoint_result = $Entities.get_next_waypoint(entity);
                if (waypoint_result.has_value()) {
                    auto [status, waypoint] = waypoint_result.value();
                    if (status == PathRequestResult::StillSearching || status == PathRequestResult::TargetReached) {
                        std::cout << fmt::format("Entity {} moving to waypoint ({}, {}) - Status: {}\n", 
                            entity.id(), waypoint.x, waypoint.y, 
                            status == PathRequestResult::StillSearching ? "StillSearching" : "TargetReached");
                        entity.set<LuaWaypoint>({static_cast<int>(waypoint.x), static_cast<int>(waypoint.y)});
                    } else if (status == PathRequestResult::TargetUnreachable) {
                        std::cout << fmt::format("Entity {} target unreachable, clearing target\n", entity.id());
                        entity.remove<LuaTarget>();
                    }
                }
            });

        world.system<LuaEntity, LuaWaypoint>()
            RUN_LOCK($Entities.entities_lock())
            .each([](flecs::entity entity, LuaEntity& entity_data, LuaWaypoint& waypoint) {
                float dt = entity.world().delta_time();
                if (dt <= 0.0f)
                    return; // Skip if no delta time

                glm::vec2 current_pos = {entity_data.x, entity_data.y};
                glm::vec2 target_pos = {static_cast<float>(waypoint.x), static_cast<float>(waypoint.y)};
                glm::vec2 direction = target_pos - current_pos;
                float distance = glm::length(direction);
                
                const float ARRIVAL_THRESHOLD = 2.0f; // World units, adjust based on your tile size
                
                if (distance < ARRIVAL_THRESHOLD) {
                    // Close enough, snap to target and remove waypoint
                    std::cout << fmt::format("Entity {} reached waypoint ({}, {})\n", entity.id(), waypoint.x, waypoint.y);
                    entity_data.x = target_pos.x;
                    entity_data.y = target_pos.y;
                    entity.remove<LuaWaypoint>();
                } else {
                    // Move towards target at constant speed
                    glm::vec2 normalized_direction = glm::normalize(direction);
                    glm::vec2 delta = normalized_direction * entity_data.speed * dt;
                    
                    // Check if we would overshoot the target
                    float movement_distance = glm::length(delta);
                    if (movement_distance >= distance) {
                        // We would overshoot, so just move directly to target
                        std::cout << fmt::format("Entity {} overshooting waypoint, snapping to ({}, {})\n", entity.id(), waypoint.x, waypoint.y);
                        entity_data.x = target_pos.x;
                        entity_data.y = target_pos.y;
                        entity.remove<LuaWaypoint>();
                    } else {
                        // Move towards target
                        entity_data.x += delta.x;
                        entity_data.y += delta.y;
                    }
                }
            });
        
        world.observer<LuaWaypoint>()
            .event(flecs::OnRemove)
            RUN_LOCK($Entities.entities_lock())
            .each([](flecs::entity entity, LuaWaypoint& waypoint) {
                // Only continue if entity still has a target
                if (!entity.has<LuaTarget>())
                    return;
                
                auto waypoint_result = $Entities.get_next_waypoint(entity);
                if (waypoint_result.has_value()) {
                    auto [status, next_waypoint] = waypoint_result.value();
                    if (status == PathRequestResult::StillSearching) {
                        std::cout << fmt::format("Entity {} continuing to next waypoint ({}, {})\n", entity.id(), next_waypoint.x, next_waypoint.y);
                        entity.set<LuaWaypoint>({static_cast<int>(next_waypoint.x), static_cast<int>(next_waypoint.y)});
                    } else if (status == PathRequestResult::TargetReached) {
                        std::cout << fmt::format("Entity {} reached target, clearing target\n", entity.id());
                        entity.remove<LuaTarget>();
                    } else if (status == PathRequestResult::TargetUnreachable) {
                        std::cout << fmt::format("Entity {} target unreachable, clearing target\n", entity.id());
                        entity.remove<LuaTarget>();
                    }
                }
            });
        
        world.observer<LuaTarget>()
            .event(flecs::OnRemove)
            RUN_LOCK($Entities.entities_lock())
            .each([](flecs::entity entity, LuaTarget& target) {
                std::cout << fmt::format("Entity {} target cleared\n", entity.id());
                $Entities.clear_target(entity);
                if (entity.has<LuaWaypoint>())
                    entity.remove<LuaWaypoint>();
            });
    }
};
