//
//  robot.hh
//  rpg
//
//  Created by George Watson on 10/08/2025.
//

#pragma once

#include "components.hh"
#include "flecs.h"
#include "fmt/format.h"
#include "glm/vec2.hpp"

class Robot {
    flecs::world *_ecs;

public:
    Robot(flecs::world *ecs, int x, int y, int chunk_x, int chunk_y) : _ecs(ecs) {
        flecs::entity chunk_entity = _ecs->entity(fmt::format("Chunk({}, {})", chunk_x, chunk_y).c_str());
        flecs::entity robot_entity = _ecs->entity("Robot")
            .child_of(chunk_entity)
            .set<Position>({x, y})
            .set<Velocity>({0, 0});
    }
};