/* main.c -- https://github.com/takeiteasy/rpg 

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

#ifndef DEFAULT_WINDOW_WIDTH
#define DEFAULT_WINDOW_WIDTH 640
#endif

#ifndef DEFAULT_WINDOW_HEIGHT
#define DEFAULT_WINDOW_HEIGHT 480
#endif

#define SETTINGS                                                                                 \
    X("width", integer, width, DEFAULT_WINDOW_WIDTH, "Set window width")                         \
    X("height", integer, height, DEFAULT_WINDOW_HEIGHT, "Set window height")                     \
    X("sampleCount", integer, sample_count, 4, "Set the MSAA sample count of the framebuffer")   \
    X("swapInterval", integer, swap_interval, 1, "Set the preferred swap interval")              \
    X("highDPI", boolean, high_dpi, true, "Enable high-dpi compatability")                       \
    X("fullscreen", boolean, fullscreen, false, "Set fullscreen")                                \
    X("alpha", boolean, alpha, false, "Enable/disable alpha channel on framebuffers")            \
    X("clipboard", boolean, enable_clipboard, false, "Enable clipboard support")                 \
    X("clipboardSize", integer, clipboard_size, 1024, "Size of clipboard buffer (in bytes)")     \
    X("drapAndDrop", boolean, enable_dragndrop, false, "Enable drag-and-drop files")             \
    X("maxDroppedFiles", integer, max_dropped_files, 1, "Max number of dropped files")           \
    X("maxDroppedFilesPathLength", integer, max_dropped_file_path_length, MAX_PATH, "Max path length for dropped files")

static void usage(const char *name, int exit_code) {
    printf("  usage: %s [options]\n\n  options:\n", name);
    printf("\t  help (flag) -- Show this message\n");
    printf("\t  config (string) -- Path to .json config file\n");
#define X(NAME, TYPE, VAL, DEFAULT, DOCS) \
    printf("\t  %s (%s) -- %s (default: %d)\n", NAME, #TYPE, DOCS, DEFAULT);
    SETTINGS
#undef X
    exit(exit_code);
}

#define X(NAME)                         \
void NAME##_init(void);                 \
void NAME##_enter(void);                \
void NAME##_exit(void);                 \
void NAME##_event(const sapp_event*);   \
void NAME##_step(void);                 \
struct scene NAME##_scene = {           \
    .name = #NAME,                      \
    .enter = NAME##_enter,              \
    .exit = NAME##_exit,                \
    .step = NAME##_step                 \
};
SCENES
#undef X

static struct {
    sapp_desc desc;
    struct scene *scene_prev, *scene_current, *next_scene;
} state = {
    .desc = (sapp_desc) {
#define X(NAME, TYPE, VAL, DEFAULT, DOCS) .VAL = DEFAULT,
        SETTINGS
#undef X
        .window_title = "rpg"
    }
};

static const char* read_file(const char *path, size_t *size) {
    FILE *fh = fopen(path, "rb");
    const char *data = NULL;
    size_t sz = -1;
    if (!fh)
        return NULL;
    if (fseek(fh, 0, SEEK_END) != 0 ||
        (sz = ftell(fh)) <= 0 ||
        fseek(fh, 0, SEEK_SET) != 0 ||
        !(data = malloc(sz + 1)))
        goto BAIL;
    if (fread((void*)data, 1, sz, fh) != sz) {
        free((void*)data);
        data = NULL;
        goto BAIL;
    }
    ((char*)data)[sz] = '\0';
BAIL:
    if (fh)
        fclose(fh);
    if (size)
        *size = sz;
    return data;
}

static int load_config(const char *path) {
    const char *data = read_file(path, NULL);
    if (data)
        return 0;

    const struct json_attr_t config_attr[] = {
#define X(NAME, TYPE, VAL, DEFAULT,DOCS) \
        {(char*)#NAME, t_##TYPE, .addr.TYPE=&state.desc.VAL},
        SETTINGS
#undef X
        {NULL}
    };
    if (!json_read_object(data, config_attr, NULL))
        return 0;
    free((void*)data);
    return 1;
}

#define jim_boolean jim_bool

static int export_config(const char *path) {
    FILE *fh = fopen(path, "w");
    if (!fh)
        return 0;
    Jim jim = {
        .sink = fh,
        .write = (Jim_Write)fwrite
    };
    jim_object_begin(&jim);
#define X(NAME, TYPE, VAL, DEFAULT, DOCS) \
    jim_member_key(&jim, NAME);           \
    jim_##TYPE(&jim, state.desc.VAL);
    SETTINGS
#undef X
    jim_object_end(&jim);
    fclose(fh);
    return 1;
}

static int parse_arguments(int argc, char *argv[]) {
    const char *name = argv[0];
    sargs_desc desc = (sargs_desc) {
#ifdef EMSCRIPTEN
        .argc = argc,
        .argv = (char**)argv
#else
        .argc = argc - 1,
        .argv = (char**)(argv + 1)
#endif
    };
    sargs_setup(&desc);

#define boolean 1
#define integer 0
#define X(NAME, TYPE, VAL, DEFAULT, DOCS)                                               \
    if (sargs_exists(NAME))                                                             \
    {                                                                                   \
        const char *tmp = sargs_value_def(NAME, #DEFAULT);                              \
        if (!tmp)                                                                       \
        {                                                                               \
            fprintf(stderr, "[ARGUMENT ERROR] No value passed for \"%s\"\n", NAME);     \
            usage(name, 1);                                                             \
            return 0;                                                                   \
        }                                                                               \
        if (TYPE == 1)                                                                  \
            state.desc.VAL = (int)atoi(tmp);                                            \
        else                                                                            \
            state.desc.VAL = sargs_boolean(NAME);                                       \
    }
    SETTINGS
#undef X
#undef boolean
#undef integer
    return 1;
}

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

#define _STRINGIFY(s) #s
#define STRINGIFY(S) _STRINGIFY(S)

static void init(void) {
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    sapp_input_init();

    set_scene_named(STRINGIFY(FIRST_SCENE));
}

static void frame(void) {
    if (!state.scene_current) {
        sapp_quit();
        return;
    }

    state.scene_current->step();
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
#ifdef PLATFORM_WINDOWS
    const char *config_path = "rpg_config.json";
#else
    const char *config_path = ".rpg_config.json";
#endif
    if (file_exists(config_path)) {
        if (!load_config(config_path)) {
            fprintf(stderr, "[IMPORT CONFIG ERROR] Failed to import config from \"%s\"\n", config_path);
            fprintf(stderr, "errno (%d): \"%s\"\n", errno, strerror(errno));
            goto EXPORT_CONFIG;
        }
    } else {
    EXPORT_CONFIG:
        if (!export_config(config_path)) {
            fprintf(stderr, "[EXPORT CONFIG ERROR] Failed to export config to \"%s\"\n", config_path);
            fprintf(stderr, "errno (%d): \"%s\"\n", errno, strerror(errno));
            abort();
        }
    }
    if (argc > 1)
        if (!parse_arguments(argc, argv)) {
            fprintf(stderr, "[PARSE ARGUMENTS ERROR] Failed to parse arguments\n");
            abort();
        }
    return (sapp_desc) {
        .init_cb = init,
        .frame_cb = frame,
        .event_cb = event,
        .cleanup_cb = cleanup
    };
}
