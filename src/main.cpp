/* https://github.com/takeiteasy/nice 

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

#include "nice.hpp"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_app.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"
#include "sokol/sokol_time.h"
#include "sokol/util/sokol_debugtext.h"
#include "imgui.h"
#include "sokol/util/sokol_imgui.h"
#include "glm/vec2.hpp"
#include "passthru.glsl.h"

#define X(NAME)                                     \
extern void NAME##_enter(void);                     \
extern void NAME##_exit(void);                      \
extern void NAME##_step(void);                      \
extern void NAME##_event(const sapp_event *event);  \
struct scene NAME##_scene = {                       \
    .name = #NAME,                                  \
    .enter = NAME##_enter,                          \
    .exit = NAME##_exit,                            \
    .step = NAME##_step,                            \
    .event = NAME##_event                           \
};
SCENES
#undef X

static struct {
    struct scene *scene_prev, *scene_current, *next_scene;
    sg_pipeline pipeline;
    sg_pass_action pass_action;
    sg_bindings bind;
    sg_pass pass;
    sg_image color, depth;
    sg_sampler sampler;
    sg_shader shader;
    int framebuffer_width = DEFAULT_WINDOW_WIDTH;
    int framebuffer_height = DEFAULT_WINDOW_HEIGHT;
} state;

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
        .width = width,
        .height = height,
        .pixel_format = SG_PIXELFORMAT_RGBA8
    };
    state.color = sg_make_image(&img_desc);
    img_desc.pixel_format = SG_PIXELFORMAT_DEPTH;
    state.depth = sg_make_image(&img_desc);
    sg_attachments_desc attr_desc = {
        .colors[0].image = state.color,
        .depth_stencil.image = state.depth
    };
    state.pass = (sg_pass) {
        .attachments = sg_make_attachments(&attr_desc),
        .action = state.pass_action
    };
    state.bind.images[IMG_tex] = state.color;
    state.bind.samplers[SMP_smp] = state.sampler;
}

uint64_t index(int _x, int _y) {
    // Map (x,y) coordinates to a unique ID
#define _INDEX(I) (abs((I) * 2) - ((I) > 0 ? 1 : 0))
    int x = _INDEX(_x), y = _INDEX(_y);
    return x >= y ? x * x + x + y : x + y * y;
#undef _INDEX
}

std::pair<int, int> unindex(uint64_t id_value) {
    // First, we need to find which (x,y) pair in the _INDEX space maps to this id
    // The formula is: id = x >= y ? x*x + x + y : x + y*y
    // We need to reverse this process
    // Find the "diagonal" we're on (similar to Cantor pairing function)
    uint64_t w = static_cast<uint64_t>(floor(sqrt(static_cast<double>(id_value))));
    uint64_t t = w * w;
    int ix, iy;
    if (id_value < t + w) {
        // We're in the lower triangle: x + y*y = id_value, where y = w
        ix = static_cast<int>(id_value - t);
        iy = static_cast<int>(w);
    } else {
        // We're in the upper triangle: x*x + x + y = id_value, where x = w
        ix = static_cast<int>(w);
        iy = static_cast<int>(id_value - t - w);
    }
    // Now convert from _INDEX space back to regular coordinates
#define _UNINDEX(I) ((I) == 0 ? 0 : ((I) % 2 == 1) ? ((I) + 1) / 2 : -((I) / 2))
    return {_UNINDEX(ix), _UNINDEX(iy)};
#undef _UNINDEX
}

struct PassThruVertex {
    glm::vec2 position;
    glm::vec2 texcoord;
};

static void init(void) {
    sg_desc desc = {
        .environment = sglue_environment(),
        .buffer_pool_size = (1<<16)-1,
        .logger.func = slog_func,
    };
    sg_setup(&desc);

    sdtx_desc_t dtx_desc = {
        .fonts = { sdtx_font_oric() }
    };
    sdtx_setup(&dtx_desc);

    stm_setup();

    state.shader = sg_make_shader(passthru_shader_desc(sg_query_backend()));

    sg_pipeline_desc pip_desc = {
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .shader = state.shader,
        .index_type = SG_INDEXTYPE_UINT16,
        .layout = {
            .buffers[0].stride = sizeof(PassThruVertex),
            .attrs = {
                [ATTR_passthru_position].format = SG_VERTEXFORMAT_FLOAT2,
                [ATTR_passthru_texcoord].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .cull_mode = SG_CULLMODE_BACK,
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true
        }
    };
    state.pipeline = sg_make_pipeline(&pip_desc);

    state.pass_action = (sg_pass_action) {
        .colors[0] = {
            .load_action = SG_LOADACTION_CLEAR,
            .clear_value = { .9f, .9f, .9f, 1.f }
        }
    };

    sg_sampler_desc smp_desc = {
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_BORDER,
        .wrap_v = SG_WRAP_CLAMP_TO_BORDER,
    };
    state.sampler = sg_make_sampler(&smp_desc);

    framebuffer_resize(state.framebuffer_width, state.framebuffer_height);

    float vertices[] = {
        -1.0f,  1.0f, 0.0f, 0.0f, // Top-left
         1.0f,  1.0f, 1.0f, 0.0f, // Top-right
         1.0f, -1.0f, 1.0f, 1.0f, // Bottom-right
        -1.0f, -1.0f, 0.0f, 1.0f, // Bottom-left
    };
    uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };
    sg_buffer_desc v_desc = {
        .data = SG_RANGE(vertices)
    };
    state.bind.vertex_buffers[0] = sg_make_buffer(&v_desc);
    sg_buffer_desc i_desc = {
        .usage.index_buffer = true,
        .data = SG_RANGE(indices)
    };
    state.bind.index_buffer = sg_make_buffer(&i_desc);

    simgui_desc_t simgui_desc = { };
    simgui_desc.logger.func = slog_func;
    simgui_setup(&simgui_desc);

#define _STRINGIFY(s) #s
#define STRINGIFY(S) _STRINGIFY(S)
    set_scene_named(STRINGIFY(FIRST_SCENE));
}

static void frame(void) {
    if (!state.scene_current) {
        sapp_quit();
        return;
    }

    int width = sapp_width();
    int height = sapp_height();
    simgui_new_frame({ width, height, sapp_frame_duration(), sapp_dpi_scale() });

    sg_begin_pass(&state.pass);
    sdtx_canvas(width, height);
    state.scene_current->step();
    sg_end_pass();
    sg_pass pass_desc = {
        .action = state.pass_action,
        .swapchain = sglue_swapchain()
    };
    sg_begin_pass(&pass_desc);
    sg_apply_pipeline(state.pipeline);
    sg_apply_bindings(&state.bind);
    sg_draw(0, 6, 1);
    sdtx_draw();
    simgui_render();
    sg_end_pass();
    sg_commit();

    if (state.next_scene) {
        if ((state.scene_prev = state.scene_current)) {
            state.scene_current->exit();
            $Assets.clear();
        }
        if ((state.scene_current = state.next_scene))
            state.scene_current->enter();
        state.next_scene = NULL;
    }
}

static void event(const sapp_event *event) {
    simgui_handle_event(event);
    if (state.scene_current != NULL)
        state.scene_current->event(event);
}

static void cleanup(void) {
    state.scene_current->exit();
    $Assets.clear();
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc) {
        .width = DEFAULT_WINDOW_WIDTH,
        .height = DEFAULT_WINDOW_HEIGHT,
        .window_title = "nice",
        .init_cb = init,
        .frame_cb = frame,
        .event_cb = event,
        .cleanup_cb = cleanup
    };
}
