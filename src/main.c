/* https://github.com/takeiteasy/rpg 

 rpg Copyright (C) 2025 George Watson

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>. */

#define SOKOL_IMPL
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_app.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_audio.h"
#include "sokol/sokol_log.h"
#include "sokol/sokol_time.h"
#include "sokol/sokol_args.h"
#define SOKOL_INPUT_NO_GAMEPADS
#include "sokol_input.h"
#define BLA_IMPLEMENTATION
#include "bla.h"
#define PAT_IMPLEMENTATION
#include "pat.h"
#define GARRY_IMPLEMENTATION
#include "garry.h"
#define TABLE_IMPLEMENTATION
#include "table.h"
#define JIM_IMPLEMENTATION
#include "jim.h"
#define MJSON_IMPLEMENTATION
#include "mjson.h"

#include "scene.h"
#include "basic.glsl.h"

#ifndef DEFAULT_WINDOW_WIDTH
#define DEFAULT_WINDOW_WIDTH 640
#endif

#ifndef DEFAULT_WINDOW_HEIGHT
#define DEFAULT_WINDOW_HEIGHT 480
#endif

#define X(NAME)                  \
extern void NAME##_enter(void);  \
extern void NAME##_exit(void);   \
extern void NAME##_step(void);   \
struct scene NAME##_scene = {    \
    .name = #NAME,               \
    .enter = NAME##_enter,       \
    .exit = NAME##_exit,         \
    .step = NAME##_step          \
};
SCENES
#undef X

static struct {
    int framebuffer_width, framebuffer_height;
    struct scene *scene_prev, *scene_current, *next_scene;
    sg_pipeline pip;
    sg_pass_action pass_action;
    sg_bindings bind;
    sg_pass pass;
    sg_image color, depth;
    sg_sampler sampler;
} state = {
    .framebuffer_width = DEFAULT_WINDOW_WIDTH,
    .framebuffer_height = DEFAULT_WINDOW_HEIGHT
};

static struct scene* find_scene(const char *name) {
    size_t name_len = strlen(name);
#define X(NAME) \
    if (!strncmp(name, #NAME, name_len)) \
        return &NAME##_scene;
    SCENES
#undef X
    fprintf(stderr, "[ERROR] Unknown scene '%s'", name);
    abort();
}

void set_scene(struct scene *scene) {
    if (!scene) {
        sapp_quit();
        return;
    }

    if (!state.scene_current) {
        state.scene_current = scene;
        state.scene_current->enter();
    } else
        if (strncmp(scene->name, state.scene_current->name, strlen(scene->name)))
            state.next_scene = scene;
}

void set_scene_named(const char *name) {
    set_scene(find_scene(name));
}

int framebuffer_width(void) {
    return state.framebuffer_width;
}

int framebuffer_height(void) {
    return state.framebuffer_height;
}

void framebuffer_resize(int width, int height) {
    if (sg_query_image_state(state.color) == SG_RESOURCESTATE_VALID)
        sg_destroy_image(state.color);
    if (sg_query_image_state(state.depth) == SG_RESOURCESTATE_VALID)
        sg_destroy_image(state.depth);
    sg_image_desc img_desc = {
        .usage.render_attachment = true,
        .width = 640,
        .height = 480,
        .pixel_format = SG_PIXELFORMAT_RGBA8
    };
    state.color = sg_make_image(&img_desc);
    img_desc.pixel_format = SG_PIXELFORMAT_DEPTH;
    state.depth = sg_make_image(&img_desc);
    state.pass = (sg_pass) {
        .attachments = sg_make_attachments(&(sg_attachments_desc) {
            .colors[0].image = state.color,
            .depth_stencil.image = state.depth
        }),
        .action = state.pass_action
    };
    state.bind.images[IMG_tex] = state.color;
    state.bind.samplers[SMP_smp] = state.sampler;
}

struct basic_vertex {
    vec2 position;
    vec2 texcoord;
};

static void init(void) {
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    sapp_input_init();

    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .shader = sg_make_shader(basic_shader_desc(sg_query_backend())),
        .index_type = SG_INDEXTYPE_UINT16,
        .layout = {
            .buffers[0].stride = sizeof(struct basic_vertex),
            .attrs = {
                [ATTR_basic_position].format = SG_VERTEXFORMAT_FLOAT2,
                [ATTR_basic_texcoord].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .cull_mode = SG_CULLMODE_BACK,
//        .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8,
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true
        }
    });

    state.pass_action = (sg_pass_action) {
        .colors[0] = {
            .load_action = SG_LOADACTION_CLEAR,
            .clear_value = { 0.0f, 0.0f, 0.0f, 1.0f }
        }
    };

    state.sampler = sg_make_sampler(&(sg_sampler_desc) {
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_REPEAT,
        .wrap_v = SG_WRAP_REPEAT,
    });

    framebuffer_resize(state.framebuffer_width, state.framebuffer_height);

    float vertices[] = {
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
    };
    uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };
    state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc) {
        .data = SG_RANGE(vertices)
    });
    state.bind.index_buffer = sg_make_buffer(&(sg_buffer_desc) {
        .usage.index_buffer = true,
        .data = SG_RANGE(indices)
    });

#define _STRINGIFY(s) #s
#define STRINGIFY(S) _STRINGIFY(S)
    set_scene_named(STRINGIFY(FIRST_SCENE));
}

static void frame(void) {
    if (!state.scene_current) {
        sapp_quit();
        return;
    }

    sg_begin_pass(&state.pass);
    state.scene_current->step();
    sg_end_pass();
    sg_begin_pass(&(sg_pass) {
        .action = state.pass_action,
        .swapchain = sglue_swapchain()
    });
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);
    sg_draw(0, 6, 1);
    sg_end_pass();
    sg_commit();
    sapp_input_flush();

    if (state.next_scene) {
        if ((state.scene_prev = state.scene_current))
            state.scene_current->exit();
        if ((state.scene_current = state.next_scene))
            state.scene_current->enter();
        state.next_scene = NULL;
    }
}

static void event(const sapp_event *event) {
    sapp_input_event(event);
}

static void cleanup(void) {
    if (state.scene_current)
        state.scene_current->exit();
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc) {
        .width = DEFAULT_WINDOW_WIDTH,
        .height = DEFAULT_WINDOW_HEIGHT,
        .window_title = "rpg",
        .init_cb = init,
        .frame_cb = frame,
        .event_cb = event,
        .cleanup_cb = cleanup
    };
}
