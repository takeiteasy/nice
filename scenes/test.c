//
//  test.c
//  rpg
//
//  Created by George Watson on 24/07/2025.
//

#include "scene.h"
#include "ecs.h"
#include "chunk.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_app.h"
#include "sokol/sokol_glue.h"
#include "sokol_input.h"
#include "default.glsl.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#define SOKOL_DEBUGTEXT_IMPL
#include "sokol/util/sokol_debugtext.h"

static struct {
    struct world world;
    struct chunk test;
    struct chunk test2;
    struct chunk test3;
    struct chunk test4;
    struct texture tilemap;
    struct camera camera;

    sg_pass_action pass_action;
    sg_pipeline pip;

    bool dragging;
} state;

void test_enter(void) {
    rng_srand(time(NULL));
    ecs_world_create(&state.world);
    state.camera = camera_create(0, 0, 1., 0);
    chunk_create(&state.test, 0, 0, 0);
    chunk_fill(&state.test);
    chunk_create(&state.test2, -1, 0, 0);
    chunk_fill(&state.test2);
    chunk_create(&state.test3, -1, -1, 0);
    chunk_fill(&state.test3);
    chunk_create(&state.test4, 0, -1, 0);
    chunk_fill(&state.test4);

    sdtx_setup(&(sdtx_desc_t){
        .fonts = {
            sdtx_font_kc853(),
//            sdtx_font_kc854(),
//            sdtx_font_z1013(),
//            sdtx_font_cpc(),
//            sdtx_font_c64(),
//            sdtx_font_oric()
        }
    });

    assert(texture_load_path(&state.tilemap, "assets/tilemap.exploded.png"));

    sg_shader shd = sg_make_shader(default_program_shader_desc(sg_query_backend()));

    state.pip = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = shd,
        .layout = {
            .buffers[0].stride = sizeof(float) * 4,
            .attrs = {
                [ATTR_default_program_position].format = SG_VERTEXFORMAT_FLOAT2,
                [ATTR_default_program_texcoord].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
    });

    state.pass_action = (sg_pass_action) {
        .colors[0] = {
            .load_action=SG_LOADACTION_CLEAR,
            .clear_value={.125f, .125f, .125f, 1.f}
        }
    };
}

void test_exit(void) {
    chunk_destroy(&state.test);
    texture_destroy(&state.tilemap);
    sg_destroy_pipeline(state.pip);
    ecs_world_destroy(&state.world);
}

void test_step(void) {
    if (sapp_was_button_pressed(SAPP_MOUSEBUTTON_LEFT) && !state.dragging)
        state.dragging = true;
    if (sapp_was_button_released(SAPP_MOUSEBUTTON_LEFT) && state.dragging)
        state.dragging = false;
    if (state.dragging) {
        HMM_Vec2 mouse_delta = HMM_V2(sapp_mouse_delta_x(), sapp_mouse_delta_y());
        float drag_scale = 1.0f / state.camera.zoom;
        mouse_delta = HMM_MulV2F(mouse_delta, drag_scale);
        camera_move(&state.camera, mouse_delta.X, mouse_delta.Y);
    }

    if (sapp_was_scrolled())
        camera_zoom(&state.camera, sapp_scroll_y() * 0.1f);

    sdtx_canvas(sapp_width()/2.0f, sapp_height()/2.0f);
    sdtx_home();
    sdtx_printf("fps:   %.2f", 1.0f / sapp_frame_duration());
    sdtx_crlf();
    sdtx_printf("pos:   (%.2f, %.2f)", state.camera.position.X, state.camera.position.Y);
    sdtx_crlf();
    sdtx_printf("zoom:  (%.2f)", state.camera.zoom);
    sdtx_crlf();
    sdtx_printf("drag:  %s", state.dragging ? "true" : "false");
    sdtx_crlf();
    sdtx_printf("mouse: (%.2d, %.2d)", sapp_mouse_x(), sapp_mouse_y());
    sdtx_crlf();
    HMM_Vec2 mouse_world = camera_screen_to_world(&state.camera, HMM_V2(sapp_mouse_x(), sapp_mouse_y()), sapp_width(), sapp_height());
    sdtx_printf("world: (%.2f, %.2f)", mouse_world.X, mouse_world.Y);
    sdtx_crlf();
    HMM_Vec2 mouse_chunk = camera_screen_to_world_chunk(&state.camera, HMM_V2(sapp_mouse_x(), sapp_mouse_y()), sapp_width(), sapp_height());
    sdtx_printf("chunk: (%d, %d)", (int)mouse_chunk.X, (int)mouse_chunk.Y);
    sdtx_crlf();
    HMM_Vec2 mouse_tile = camera_screen_to_world_tile(&state.camera, HMM_V2(sapp_mouse_x(), sapp_mouse_y()), sapp_width(), sapp_height());
    sdtx_printf("tile:  (%d, %d)", (int)mouse_tile.X, (int)mouse_tile.Y);

    sg_begin_pass(&(sg_pass) {
        .action = state.pass_action,
        .swapchain = sglue_swapchain()
    });
    sg_apply_pipeline(state.pip);
    chunk_draw(&state.test, &state.tilemap, &state.camera);
    chunk_draw(&state.test2, &state.tilemap, &state.camera);
    chunk_draw(&state.test3, &state.tilemap, &state.camera);
    chunk_draw(&state.test4, &state.tilemap, &state.camera);
    sdtx_draw();
    sg_end_pass();
    sg_commit();
    state.camera.dirty = false;
}
