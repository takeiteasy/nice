//
//  test.c
//  rpg
//
//  Created by George Watson on 24/07/2025.
//

#include "scene.hh"
#include "map.hh"
#include "sokol/util/sokol_debugtext.h"
#include "sokol_input.h"

static struct {
    Map *map;
    bool dragging;
} state;

void test_enter(void) {
    state.map = new Map();
    state.dragging = false;
}

void test_exit(void) {
    if (state.map)
        delete state.map;
}

void test_step(void) {
    if (sapp_was_button_pressed(SAPP_MOUSEBUTTON_LEFT) && !state.dragging)
        state.dragging = true;
    if (sapp_was_button_released(SAPP_MOUSEBUTTON_LEFT) && state.dragging)
        state.dragging = false;
    if (state.dragging)
        state.map->camera()->move_by(glm::vec2(-sapp_mouse_delta_x(), -sapp_mouse_delta_y()) * (1.0f / state.map->camera()->zoom()));

    if (sapp_was_scrolled())
        state.map->camera()->zoom_by(sapp_scroll_y() * .1f);

    sdtx_home();
    sdtx_printf("fps:    %.2f", 1.0f / sapp_frame_duration());
    sdtx_crlf();
    sdtx_printf("pos:    (%.2f, %.2f)", state.map->camera()->position().x, state.map->camera()->position().y);
    sdtx_crlf();
    sdtx_printf("zoom:   (%.2f)", state.map->camera()->zoom());
    sdtx_crlf();
    sdtx_printf("drag:   %s", state.dragging ? "true" : "false");
    sdtx_crlf();
    glm::vec2 mouse_pos = glm::vec2(sapp_mouse_x(), sapp_mouse_y());
    sdtx_printf("mouse:  (%.2f, %.2f)", mouse_pos.x, mouse_pos.y);
    sdtx_crlf();
    glm::vec2 mouse_world = state.map->camera()->screen_to_world(mouse_pos);
    sdtx_printf("world:  (%.2f, %.2f)", mouse_world.x, mouse_world.y);
    sdtx_crlf();
    glm::vec2 mouse_chunk = state.map->camera()->world_to_chunk(mouse_world);
    sdtx_printf("chunk:  (%d, %d)", (int)mouse_chunk.x, (int)mouse_chunk.y);
    sdtx_crlf();
    glm::vec2 mouse_tile = state.map->camera()->world_to_tile(mouse_world);
    sdtx_printf("tile:   (%d, %d)", (int)mouse_tile.x, (int)mouse_tile.y);
    sdtx_crlf();
    Rect bounds = state.map->camera()->bounds();
    sdtx_printf("camera: (%d, %d, %d, %d)", bounds.x, bounds.y, bounds.x + bounds.w, bounds.y + bounds.h);
    sdtx_crlf();
    bounds = state.map->camera()->max_bounds();
    sdtx_printf("camera: (%d, %d, %d, %d)", bounds.x, bounds.y, bounds.x + bounds.w, bounds.y + bounds.h);
    sdtx_crlf();

    int chunks_drawn = state.map->draw();
    sdtx_printf("chunks drawn: %d", chunks_drawn);
}
