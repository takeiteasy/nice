//
//  test.c
//  rpg
//
//  Created by George Watson on 24/07/2025.
//

#include "scene.hh"
#include "assets.hh"
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/util/sokol_debugtext.h"
#include "sokol_input.h"
#include "chunk_factory.hh"

static struct {
    Camera *camera;
    ChunkFactory *manager;
    bool camera_dragging = false;
} state;

void test_enter(void) {
    state.camera = new Camera();
    $Assets.load<Texture>("tilemap", "assets/tilemap.exploded.png");
    $Assets.load<Texture>("robot", "assets/man.exploded.png");
    state.manager = new ChunkFactory(state.camera);
}

void test_exit(void) {
    delete state.manager;
    delete state.camera;
}

void test_step(void) {
    if ((sapp_was_button_pressed(SAPP_MOUSEBUTTON_LEFT) && sapp_modifier_equals(SAPP_MODIFIER_SHIFT)) && !state.camera_dragging)
        state.camera_dragging = true;
    if (sapp_was_button_released(SAPP_MOUSEBUTTON_LEFT) && state.camera_dragging)
        state.camera_dragging = false;
    if (state.camera_dragging)
        state.camera->move_by(glm::vec2(-sapp_mouse_delta_x(), -sapp_mouse_delta_y()) * (1.f / state.camera->zoom()));

    if (sapp_was_scrolled() && sapp_modifier_equals(SAPP_MODIFIER_SHIFT))
        state.camera->zoom_by(sapp_scroll_y() * .1f);

    sdtx_home();
    sdtx_printf("fps:    %.2f\n", 1.f / sapp_frame_duration());
    sdtx_printf("pos:    (%.2f, %.2f)\n", state.camera->position().x, state.camera->position().y);
    sdtx_printf("zoom:   (%.2f)\n", state.camera->zoom());
    sdtx_printf("drag:   %s\n", state.camera_dragging ? "true" : "false");
    glm::vec2 mouse_pos = glm::vec2(sapp_mouse_x(), sapp_mouse_y());
    sdtx_printf("mouse:  (%.2f, %.2f)\n", mouse_pos.x, mouse_pos.y);
    glm::vec2 mouse_world = state.camera->screen_to_world(mouse_pos);
    sdtx_printf("world:  (%.2f, %.2f)\n", mouse_world.x, mouse_world.y);
    glm::vec2 mouse_chunk = state.camera->world_to_chunk(mouse_world);
    sdtx_printf("chunk:  (%d, %d)\n", (int)mouse_chunk.x, (int)mouse_chunk.y);
    glm::vec2 mouse_tile = state.camera->world_to_tile(mouse_world);
    sdtx_printf("tile:   (%d, %d)\n", (int)mouse_tile.x, (int)mouse_tile.y);
    Rect bounds = state.camera->bounds();
    sdtx_printf("camera: (%d, %d, %d, %d)\n", bounds.x, bounds.y, bounds.x + bounds.w, bounds.y + bounds.h);
    bounds = state.camera->max_bounds();
    sdtx_printf("camera: (%d, %d, %d, %d)\n", bounds.x, bounds.y, bounds.x + bounds.w, bounds.y + bounds.h);

    state.manager->update_chunks();
    state.manager->scan_for_chunks();
    state.manager->release_chunks();
    state.manager->draw_chunks();
}
