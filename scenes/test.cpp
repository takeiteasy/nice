//
//  test.c
//  rpg
//
//  Created by George Watson on 24/07/2025.
//

#include "scene.hpp"
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/util/sokol_debugtext.h"
#include "chunk_manager.hpp"
#include "cursor.hpp"

static struct {
    Camera *camera;
    ChunkManager *manager;
    bool camera_dragging = false;
    glm::vec2 mouse_position;
    Cursor *cursor;
} state;

void test_enter(void) {
    $Assets.set_archive("assets/assets.zip");
    state.camera = new Camera();
    state.manager = new ChunkManager(state.camera);
    state.cursor = new Cursor();
    sapp_show_mouse(false);
}

void test_exit(void) {
    delete state.cursor;
    delete state.manager;
    delete state.camera;
}

void test_event(const sapp_event *event) {
    switch (event->type) {
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            if (!state.camera_dragging) 
                state.camera_dragging = true;
            break;
        case SAPP_EVENTTYPE_MOUSE_UP:
            if (state.camera_dragging)
                state.camera_dragging = false;
            break;
        case SAPP_EVENTTYPE_MOUSE_MOVE: {
            state.mouse_position = glm::vec2(event->mouse_x, event->mouse_y);
            if (state.camera_dragging)
                state.camera->move_by(-glm::vec2(event->mouse_dx, event->mouse_dy) * (1.f / state.camera->zoom()));
            state.cursor->set_position(state.mouse_position);
            break;
        }
        case SAPP_EVENTTYPE_MOUSE_SCROLL:
            state.camera->zoom_by(event->scroll_y * .1f);
            break;
        case SAPP_EVENTTYPE_MOUSE_ENTER:
            sapp_show_mouse(false);
            break;
        case SAPP_EVENTTYPE_MOUSE_LEAVE:
            sapp_show_mouse(true);
            break;
        default:
            break;
    }
}

void test_step(void) {
    sdtx_home();
    sdtx_printf("fps:    %.2f\n", 1.f / sapp_frame_duration());
    sdtx_printf("pos:    (%.2f, %.2f)\n", state.camera->position().x, state.camera->position().y);
    sdtx_printf("zoom:   %.2f\n", state.camera->zoom());
    sdtx_printf("drag:   %s\n", state.camera_dragging ? "true" : "false");
    sdtx_printf("mouse:  (%.2f, %.2f)\n", state.mouse_position.x, state.mouse_position.y);
    glm::vec2 mouse_world = state.camera->screen_to_world(state.mouse_position);
    sdtx_printf("world:  (%.2f, %.2f)\n", mouse_world.x, mouse_world.y);
    glm::vec2 mouse_chunk = state.camera->world_to_chunk(mouse_world);
    sdtx_printf("chunk:  (%d, %d)\n", (int)mouse_chunk.x, (int)mouse_chunk.y);
    glm::vec2 mouse_tile = state.camera->world_to_tile(mouse_world);
    sdtx_printf("tile:   (%d, %d)\n", (int)mouse_tile.x, (int)mouse_tile.y);
    Rect bounds = state.camera->bounds();
    sdtx_printf("camera: (%d, %d, %d, %d)\n", bounds.x, bounds.y, bounds.x + bounds.w, bounds.y + bounds.h);

    state.manager->update_chunks();
    state.manager->scan_for_chunks();
    state.manager->release_chunks();
    state.manager->draw_chunks();
    state.cursor->draw();
}
