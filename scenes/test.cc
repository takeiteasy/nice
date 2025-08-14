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
#include "chunk_manager.hh"

static struct {
    ChunkManager *manager;
} state;

void test_enter(void) {
    state.manager = new ChunkManager();
}

void test_exit(void) {
    if (state.manager)
        delete state.manager;
}

void test_step(void) {
}
