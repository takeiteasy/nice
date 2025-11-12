//
//  chunk_entity.hpp
//  nice
//
//  Created by George Watson on 06/09/2025.
//

#pragma once

#include "entity_factory.hpp"

ECS_STRUCT(LuaChunkEntity, {
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
    float speed;
});

struct LuaChunkXY {
    uint32_t x;
    uint32_t y;
};

struct LuaTarget {
    int x;
    int y;
};

struct LuaWaypoint {
    int x;
    int y;
};

struct PathRequest {
    flecs::entity entity;
    LuaChunkXY chunk;
    glm::vec2 start;
    glm::vec2 end;
};

enum PathRequestResult {
    StillSearching,
    TargetUnreachable,
    TargetReached
};

class ChunkEntityFactory: public EntityFactory<LuaChunkEntity> {
    JobQueue<PathRequest> _path_request_queue;
    std::unordered_map<flecs::entity, std::pair<PathRequestResult, std::vector<glm::vec2>>> _waypoints;
    mutable std::mutex _waypoints_mutex;
    UnorderedSet<flecs::entity> _entities_requesting_paths;

public:
    ChunkEntityFactory()
    : EntityFactory<LuaChunkEntity>()
    , _path_request_queue([&](PathRequest request) {
        std::cout << fmt::format("Processing path request for entity {}\n", request.entity.id());
        std::optional<std::vector<glm::vec2>> path = std::nullopt;

        $Chunks.get_chunk(request.chunk.x, request.chunk.y, [&](Chunk *c) {
            if (!c)
                return;
            // Convert world coordinates to tile coordinates within the chunk
            glm::vec2 start_world = request.start;
            glm::vec2 end_world = request.end;
            // Convert to chunk-local tile coordinates
            glm::vec2 start_tile = Camera::world_to_tile(start_world);
            glm::vec2 end_tile = Camera::world_to_tile(end_world);
            // Get tile coordinates relative to chunk
            int start_x = static_cast<int>(start_tile.x) % CHUNK_WIDTH;
            int start_y = static_cast<int>(start_tile.y) % CHUNK_HEIGHT;
            int end_x = static_cast<int>(end_tile.x) % CHUNK_WIDTH;
            int end_y = static_cast<int>(end_tile.y) % CHUNK_HEIGHT;
            // Ensure coordinates are within chunk bounds
            start_x = glm::clamp(start_x, 0, CHUNK_WIDTH - 1);
            start_y = glm::clamp(start_y, 0, CHUNK_HEIGHT - 1);
            end_x = glm::clamp(end_x, 0, CHUNK_WIDTH - 1);
            end_y = glm::clamp(end_y, 0, CHUNK_HEIGHT - 1);
            path = c->astar(glm::vec2(start_x, start_y), glm::vec2(end_x, end_y));
            // Convert path back to world coordinates
            if (path.has_value())
                for (auto& point : path.value()) {
                    glm::vec2 world_pos = Camera::tile_to_world(request.chunk.x, request.chunk.y,
                                                                static_cast<int>(point.x), static_cast<int>(point.y));
                    point = world_pos;
                }
        });

        std::lock_guard<std::mutex> lock(_waypoints_mutex);
        if (!path.has_value() || path->empty()) {
            std::cout << fmt::format("Pathfinding failed for entity {}\n", request.entity.id());
            _waypoints[request.entity].first = PathRequestResult::TargetUnreachable;
            _entities_requesting_paths.erase(request.entity);
        } else {
            std::cout << fmt::format("Pathfinding succeeded for entity {}, found path with {} points\n",
                                   request.entity.id(), path->size());
            _waypoints[request.entity].first = PathRequestResult::StillSearching;

            // Store up to 3 waypoints ahead for caching
            int max_cache = std::min(3, static_cast<int>(path->size()));
            auto& waypoint_cache = _waypoints[request.entity].second;
            // Don't clear the cache - append new waypoints to existing ones

            for (int i = 1; i < max_cache + 1 && i < path->size(); ++i) { // Skip first point (current position)
                waypoint_cache.push_back(path->at(i));
                std::cout << fmt::format("Cached waypoint {}: ({}, {})\n", i, path->at(i).x, path->at(i).y);
            }

            // Check if the cached waypoints include the final target or if we're close enough
            bool target_reached = false;
            if (!waypoint_cache.empty()) {
                glm::vec2 last_cached = waypoint_cache.back();
                target_reached = (glm::distance(last_cached, request.end) < 1.0f) ||
                                (path->size() <= max_cache + 1);  // +1 because we skip the first point
            }

            if (target_reached) {
                _waypoints[request.entity].first = PathRequestResult::TargetReached;
                _entities_requesting_paths.erase(request.entity);
                std::cout << fmt::format("Pathfinding completed for entity {}, target reached\n", request.entity.id());
            } else {
                // Continue pathfinding from the last cached waypoint
                glm::vec2 continuation_start = path->at(max_cache);
                glm::vec2 target_chunk_pos = Camera::world_to_chunk(request.end);
                LuaChunkXY target_chunk = {static_cast<uint32_t>(target_chunk_pos.x), static_cast<uint32_t>(target_chunk_pos.y)};

                _path_request_queue.enqueue({request.entity, target_chunk, continuation_start, request.end});
                std::cout << fmt::format("Pathfinding partial for entity {}, continuing search from ({}, {})\n",
                                       request.entity.id(), continuation_start.x, continuation_start.y);
            }
        }
    }) {}

    void add_entity_target(flecs::entity entity, LuaChunkXY chunk, LuaTarget target) {
        std::lock_guard<std::mutex> lock(_waypoints_mutex);
        _waypoints[entity] = {PathRequestResult::StillSearching, {}};
        _entities_requesting_paths.insert(entity);

        // Get entity's current position
        LuaChunkEntity *entity_data = entity.get_mut<LuaChunkEntity>();
        if (!entity_data) {
            std::cerr << "Warning: ChunkEntity missing LuaChunkEntity component in add_entity_target\n";
            return;
        }

        // Convert target tile coordinates to world coordinates
        glm::vec2 target_world = Camera::tile_to_world(chunk.x, chunk.y, target.x, target.y);

        // Current position is already in world coordinates
        glm::vec2 start_world = {entity_data->x, entity_data->y};

        std::cout << fmt::format("Setting target for entity {} from ({}, {}) to ({}, {}) in world coords\n",
                                entity.id(), start_world.x, start_world.y, target_world.x, target_world.y);

        // Enqueue the path request with proper world coordinates
        _path_request_queue.enqueue({entity, chunk, start_world, target_world});
    }

    std::optional<std::pair<PathRequestResult, glm::vec2>> get_next_waypoint(flecs::entity entity) {
        std::lock_guard<std::mutex> lock(_waypoints_mutex);
        auto it = _waypoints.find(entity);
        if (it == _waypoints.end() || it->second.second.empty())
            return std::nullopt;

        glm::vec2 next = it->second.second.front();
        it->second.second.erase(it->second.second.begin());

        PathRequestResult status = it->second.first;

        // If this is the last waypoint and pathfinding is complete, return TargetReached
        if (it->second.second.empty() && status == PathRequestResult::TargetReached) {
            _waypoints.erase(it);
            return {{PathRequestResult::TargetReached, next}};
        }

        // If this is the last waypoint but pathfinding is still in progress, keep searching
        if (it->second.second.empty() && status == PathRequestResult::StillSearching)
            return {{PathRequestResult::StillSearching, next}};

        // Clean up if target is unreachable
        if (it->second.second.empty() && status == PathRequestResult::TargetUnreachable) {
            _waypoints.erase(it);
            return {{PathRequestResult::TargetUnreachable, next}};
        }

        // More waypoints available, keep searching
        return {{PathRequestResult::StillSearching, next}};
    }

    void clear_target(flecs::entity entity) {
        std::lock_guard<std::mutex> lock(_waypoints_mutex);
        _waypoints.erase(entity);
        _entities_requesting_paths.erase(entity);
    }

    void update_entity(flecs::entity entity, LuaChunkEntity& entity_data, LuaChunkXY& chunk, bool lock=true) {
        std::shared_lock<std::shared_mutex> unlock;
        if (lock)
            unlock = std::shared_lock<std::shared_mutex>(_entities_lock);
        EntityFactory<LuaChunkEntity>::update_entity(entity, entity_data, false);
        glm::vec2 world = Camera::tile_to_world(chunk.x, chunk.y, entity_data.x, entity_data.y);
        entity_data.x = world.x;
        entity_data.y = world.y;
        if (lock)
            unlock.unlock();
    }
};


struct ChunkEntity {
    static ChunkEntityFactory* get_chunk_entity_factory_world(ecs_world_t *world) {
        lua_State *L = ecs_lua_get_state(world);
        lua_getfield(L, LUA_REGISTRYINDEX, "__chunk_entities__");
        ChunkEntityFactory* chunk_entities = static_cast<ChunkEntityFactory*>(lua_touserdata(L, -1));
        lua_pop(L, 1); // Remove the userdata from stack
        if (!chunk_entities) {
            std::cout << "ChunkEntityFactory instance not found in Lua registry\n";
            return nullptr;
        }
        return chunk_entities;
    }

    static ChunkEntityFactory* get_chunk_entity_factory(flecs::entity entity) {
        return get_chunk_entity_factory_world(entity.world().c_ptr());
    }

    ChunkEntity(flecs::world &world) {
        world.module<ChunkEntity>();
        ecs_world_t *w = world.c_ptr();
        ecs_entity_t scope = ecs_set_scope(w, 0);
        ECS_META_COMPONENT(w, LuaChunkEntity);
        ecs_set_scope(w, scope);

        world.component<LuaChunkXY>();
        world.component<LuaTarget>();
        world.component<LuaWaypoint>();

        // Set up observers to automatically manage entity_data entities
        world.observer<LuaChunkEntity>()
            .event(flecs::OnAdd)
            .each([](flecs::entity entity, LuaChunkEntity& entity_data) {
                // Set default values if they haven't been set
                if (entity_data.scale_x == 0.0f)
                    entity_data.scale_x = 1.0f;
                if (entity_data.scale_y == 0.0f)
                    entity_data.scale_y = 1.0f;
                if (entity_data.speed == 0.0f)
                    entity_data.speed = 100.0f;
                glm::vec2 chunk = Camera::world_to_chunk(glm::vec2(entity_data.x, entity_data.y));
                entity.set<LuaChunkXY>({static_cast<uint32_t>(chunk.x), static_cast<uint32_t>(chunk.y)});
                ChunkEntityFactory *chunk_entities = get_chunk_entity_factory(entity);
                chunk_entities->add_entity(entity);
            });

        
        auto run_lock = [](flecs::iter_t *it) {
            ChunkEntityFactory *chunk_entities = get_chunk_entity_factory_world(it->world);
            std::lock_guard<std::shared_mutex> lock(chunk_entities->get_lock());
            while (ecs_iter_next(it))
                it->callback(it);
        };

        world.observer<LuaChunkEntity>()
            .event(flecs::OnRemove)
            .run(run_lock)
            .each([](flecs::entity entity, LuaChunkEntity& entity_data) {
                ChunkEntityFactory *chunk_entities = get_chunk_entity_factory(entity);
                chunk_entities->remove_entity(entity, false); // lock=false since run_lock already holds the lock
            });

        world.observer<LuaChunkEntity, LuaChunkXY>()
            .event(flecs::OnSet)
            .run(run_lock)
            .each([](flecs::entity entity, LuaChunkEntity& entity_data, LuaChunkXY& chunk) {
                ChunkEntityFactory *chunk_entities = get_chunk_entity_factory(entity);
                chunk_entities->update_entity(entity, entity_data, chunk, false);
            });

        world.observer<LuaChunkXY, LuaTarget>()
            .event(flecs::OnSet)
            .run(run_lock)
            .each([](flecs::entity entity, LuaChunkXY& chunk, LuaTarget& target) {
                std::cout << fmt::format("ChunkEntity {} set target to ({}, {})\n", entity.id(), target.x, target.y);
                ChunkEntityFactory *chunk_entities = get_chunk_entity_factory(entity);
                chunk_entities->clear_target(entity);
                chunk_entities->add_entity_target(entity, chunk, target);
            });
        
        // System to assign next waypoint when entity doesn't have one but has a target
        world.system<LuaTarget>("AssignWaypoint")
            .without<LuaWaypoint>()
            .run(run_lock)
            .each([](flecs::entity entity, LuaTarget& target) {
                ChunkEntityFactory *chunk_entities = get_chunk_entity_factory(entity);
                auto waypoint_result = chunk_entities->get_next_waypoint(entity);
                if (waypoint_result.has_value()) {
                    auto [status, waypoint] = waypoint_result.value();
                    if (status == PathRequestResult::StillSearching || status == PathRequestResult::TargetReached) {
                        std::cout << fmt::format("ChunkEntity {} moving to waypoint ({}, {}) - Status: {}\n", 
                            entity.id(), waypoint.x, waypoint.y, 
                            status == PathRequestResult::StillSearching ? "StillSearching" : "TargetReached");
                        entity.set<LuaWaypoint>({static_cast<int>(waypoint.x), static_cast<int>(waypoint.y)});
                    } else if (status == PathRequestResult::TargetUnreachable) {
                        std::cout << fmt::format("ChunkEntity {} target unreachable, clearing target\n", entity.id());
                        entity.remove<LuaTarget>();
                    }
                }
            });

        world.system<LuaChunkEntity, LuaWaypoint>()
            .run(run_lock)
            .each([](flecs::entity entity, LuaChunkEntity& entity_data, LuaWaypoint& waypoint) {
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
                    std::cout << fmt::format("ChunkEntity {} reached waypoint ({}, {})\n", entity.id(), waypoint.x, waypoint.y);
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
                        std::cout << fmt::format("ChunkEntity {} overshooting waypoint, snapping to ({}, {})\n", entity.id(), waypoint.x, waypoint.y);
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
            .run(run_lock)
            .each([](flecs::entity entity, LuaWaypoint& waypoint) {
                // Only continue if entity still has a target
                if (!entity.has<LuaTarget>())
                    return;
                
                ChunkEntityFactory *chunk_entities = get_chunk_entity_factory(entity);
                auto waypoint_result = chunk_entities->get_next_waypoint(entity);
                if (waypoint_result.has_value()) {
                    auto [status, next_waypoint] = waypoint_result.value();
                    if (status == PathRequestResult::StillSearching) {
                        std::cout << fmt::format("ChunkEntity {} continuing to next waypoint ({}, {})\n", entity.id(), next_waypoint.x, next_waypoint.y);
                        entity.set<LuaWaypoint>({static_cast<int>(next_waypoint.x), static_cast<int>(next_waypoint.y)});
                    } else if (status == PathRequestResult::TargetReached) {
                        std::cout << fmt::format("ChunkEntity {} reached target, clearing target\n", entity.id());
                        entity.remove<LuaTarget>();
                    } else if (status == PathRequestResult::TargetUnreachable) {
                        std::cout << fmt::format("ChunkEntity {} target unreachable, clearing target\n", entity.id());
                        entity.remove<LuaTarget>();
                    }
                }
            });
        
        world.observer<LuaTarget>()
            .event(flecs::OnRemove)
            .run(run_lock)
            .each([](flecs::entity entity, LuaTarget& target) {
                std::cout << fmt::format("ChunkEntity {} target cleared\n", entity.id());
                ChunkEntityFactory *chunk_entities = get_chunk_entity_factory(entity);
                chunk_entities->clear_target(entity);
                if (entity.has<LuaWaypoint>())
                    entity.remove<LuaWaypoint>();
            });
    }
};
