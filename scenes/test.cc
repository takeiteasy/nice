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
#include "camera.hh"
#include "chunk.hh"
#include "flecs.h"
#include <thread>

static struct {
    flecs::world world;
} state;

void test_enter(void) {
    state.world.set_target_fps(60.f);
    state.world.set_threads(std::thread::hardware_concurrency());
}

void test_exit(void) {
    // ...
}

void test_step(void) {
    if (!state.world.progress())
        sapp_quit();
}
