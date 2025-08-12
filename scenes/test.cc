//
//  test.c
//  rpg
//
//  Created by George Watson on 24/07/2025.
//

#include "scene.hh"
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/util/sokol_debugtext.h"
#include "sokol_input.h"
#include "flecs.h"
#include "components.hh"
#include <thread>

static struct {
    flecs::world *ecs;
    bool dragging;
} state;

void test_enter(void) {
    state.ecs = new flecs::world();
    state.ecs->set_target_fps(60);
    state.ecs->set_threads(std::thread::hardware_concurrency());

    auto camera_prefab = state.ecs->prefab("Camera")
        .set<Position>({0, 0})
        .set<Zoom>(1.f);
    auto camera = state.ecs->entity("MainCamera")
        .is_a(camera_prefab);
}

void test_exit(void) {
    delete state.ecs;
}

void test_step(void) {
    if (!state.ecs->progress())
        sapp_quit();
}
