//
//  renderable.hpp
//  ice
//
//  Created by George Watson on 28/08/2025.
//

#pragma once

#include "flecs.h"

struct LuaRenderable {
    bool visible = true;
    int render_layer = 0;
};

// This is the module struct that gets imported
struct Renderable {
    Renderable(flecs::world &world);
};
