//
//  components.hpp
//  ice
//
//  Created by George Watson on 29/08/2025.
//

#pragma once

#include "flecs.h"

ECS_STRUCT(LuaChunk, {
    uint32_t x;
    uint32_t y;
});

ECS_STRUCT(LuaRenderable, {
    float x;
    float y;
    float width;
    float height;
    uint32_t texture_id;
    uint32_t z_index;
    float rotation;
    float scale_x;
    float scale_y;
});
