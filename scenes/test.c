//
//  test.c
//  rpg
//
//  Created by George Watson on 24/07/2025.
//

#include "scene.h"
#include "ecs.h"
#include "map.h"
#include "sokol/util/sokol_debugtext.h"
#include "sokol/sokol_app.h"
#include "sokol/sokol_glue.h"
#include "sokol_input.h"

static struct {
    struct world world;
    struct map map;
    bool dragging;
} state;

void test_enter(void) {
    rng_srand(time(NULL));
    ecs_world_create(&state.world);
    map_create(&state.map);
}

void test_exit(void) {
    map_destroy(&state.map);
    ecs_world_destroy(&state.world);
}

void test_step(void) {
    if (sapp_was_button_pressed(SAPP_MOUSEBUTTON_LEFT) && !state.dragging)
        state.dragging = true;
    if (sapp_was_button_released(SAPP_MOUSEBUTTON_LEFT) && state.dragging)
        state.dragging = false;
    if (state.dragging) {
        HMM_Vec2 mouse_delta = HMM_V2(sapp_mouse_delta_x(), sapp_mouse_delta_y());
        float drag_scale = 1.0f / state.map.camera.zoom;
        mouse_delta = HMM_MulV2F(mouse_delta, drag_scale);
        camera_move(&state.map.camera, mouse_delta.X, mouse_delta.Y);
    }

    if (sapp_was_scrolled())
        camera_zoom(&state.map.camera, sapp_scroll_y() * 0.1f);

    sdtx_home();
    sdtx_printf("fps:    %.2f", 1.0f / sapp_frame_duration());
    sdtx_crlf();
    sdtx_printf("pos:    (%.2f, %.2f)", state.map.camera.position.X, state.map.camera.position.Y);
    sdtx_crlf();
    sdtx_printf("zoom:   (%.2f)", state.map.camera.zoom);
    sdtx_crlf();
    sdtx_printf("drag:   %s", state.dragging ? "true" : "false");
    sdtx_crlf();
    HMM_Vec2 mouse_pos = HMM_V2(sapp_mouse_x(), sapp_mouse_y());
    sdtx_printf("mouse:  (%.2f, %.2f)", mouse_pos.X, mouse_pos.Y);
    sdtx_crlf();
    HMM_Vec2 mouse_world = camera_screen_to_world(&state.map.camera, mouse_pos);
    sdtx_printf("world:  (%.2f, %.2f)", mouse_world.X, mouse_world.Y);
    sdtx_crlf();
    HMM_Vec2 mouse_chunk = world_to_chunk(mouse_world);
    sdtx_printf("chunk:  (%d, %d)", (int)mouse_chunk.X, (int)mouse_chunk.Y);
    sdtx_crlf();
    HMM_Vec2 mouse_tile = world_to_tile(mouse_world);
    sdtx_printf("tile:   (%d, %d)", (int)mouse_tile.X, (int)mouse_tile.Y);
    sdtx_crlf();
    struct rect bounds = camera_bounds(&state.map.camera);
    sdtx_printf("camera: (%d, %d, %d, %d)", bounds.X, bounds.Y, bounds.X + bounds.W, bounds.Y + bounds.H);

    map_draw(&state.map);
    state.map.camera.dirty = false;
}
