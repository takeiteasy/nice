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

static struct {
    struct world world;
    struct chunk test;
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

    assert(texture_load_path(&state.tilemap, "assets/tilemap.exploded.png"));

    sg_shader shd = sg_make_shader(default_program_shader_desc(sg_query_backend()));

    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {
            .buffers[0].stride = sizeof(float) * 8,
            .attrs = {
                [ATTR_default_program_position].format = SG_VERTEXFORMAT_FLOAT2,
                [ATTR_default_program_texcoord].format = SG_VERTEXFORMAT_FLOAT2,
                [ATTR_default_program_color].format    = SG_VERTEXFORMAT_FLOAT4
            }
        },
    });

    state.pass_action = (sg_pass_action) {
        .colors[0] = {
            .load_action=SG_LOADACTION_CLEAR,
            .clear_value={1.0f, 0.0f, 0.0f, 1.0f}
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
        HMM_Vec2 mouse_delta = HMM_V2(sapp_cursor_delta_x(), sapp_cursor_delta_y());
        camera_move(&state.camera, mouse_delta.X, mouse_delta.Y);
    }

    sg_begin_pass(&(sg_pass) {
        .action = state.pass_action,
        .swapchain = sglue_swapchain()
    });
    sg_apply_pipeline(state.pip);
    chunk_draw(&state.test, &state.tilemap, &state.camera);
    sg_end_pass();
    sg_commit();
    state.camera.dirty = false;
}
