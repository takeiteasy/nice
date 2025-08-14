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
    Camera *camera;
    Texture *tilemap;
    ChunkManager *manager;
} state;

void test_enter(void) {
    state.camera = new Camera();
    state.tilemap = new Texture("assets/tilemap.png");
    state.manager = new ChunkManager(state.camera, state.tilemap);
}

void test_exit(void) {
    delete state.manager;
    delete state.tilemap;
    delete state.camera;
}

void test_step(void) {
//    state.manager->update_chunks();
    state.manager->scan_for_chunks();
}
