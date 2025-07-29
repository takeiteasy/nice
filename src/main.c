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
#include "default.glsl.h"

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
    struct scene *scene_prev, *scene_current, *next_scene;
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


static void init(void) {
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    sapp_input_init();

#define _STRINGIFY(s) #s
#define STRINGIFY(S) _STRINGIFY(S)
    set_scene_named(STRINGIFY(FIRST_SCENE));
}

static void frame(void) {
    if (!state.scene_current) {
        sapp_quit();
        return;
    }

    state.scene_current->step();

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
