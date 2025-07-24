//
//  test.c
//  rpg
//
//  Created by George Watson on 24/07/2025.
//

#include "scene.h"
#include "map.h"
#include "camera.h"
#include <stdio.h>

static struct {
    struct map map;
    struct camera camera;
} state;

void test_enter(void) {
    map_create(&state.map, "assets/tilemap.exploded.png");
    camera_create(0, 0, 1.f, 0.f);
}

void test_exit(void) {
    map_destroy(&state.map);
}

void test_step(void) {
    if (state.camera.dirty)
        camera_update_mvp(&state.camera, sapp_width(), sapp_height());
}
