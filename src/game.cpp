//
//  game.cpp
//  nice
//
//  Created by George Watson on 24/07/2025.
//

#include "nice.hpp"
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/util/sokol_debugtext.h"
#include "world.hpp"

static struct {
    Camera *camera;
    World *manager;
} state;

World* get_world() {
    return state.manager;
}

void game_enter(void) {
    $Assets.set_archive("test/assets.nice");
    state.camera = new Camera();
    state.manager = new World(state.camera);
}

void game_exit(void) {
    delete state.manager;
    delete state.camera;
}

void game_event(const sapp_event *event) {
    (void)event;
}

void game_step(void) {
    sdtx_home();
    sdtx_printf("fps:    %.2f\n", 1.f / sapp_frame_duration());
    sdtx_printf("pos:    (%.2f, %.2f)\n", state.camera->position().x, state.camera->position().y);
    sdtx_printf("zoom:   %.2f\n", state.camera->zoom());
    glm::vec2 mouse_position = $Input.mouse_position();
    sdtx_printf("mouse:  (%.2f, %.2f)\n", mouse_position.x, mouse_position.y);
    glm::vec2 mouse_world = state.camera->screen_to_world(mouse_position);
    sdtx_printf("world:  (%.2f, %.2f)\n", mouse_world.x, mouse_world.y);
    glm::vec2 mouse_chunk = Camera::world_to_chunk(mouse_world);
    sdtx_printf("chunk:  (%d, %d)\n", (int)mouse_chunk.x, (int)mouse_chunk.y);
    glm::vec2 mouse_tile = Camera::world_to_tile(mouse_world);
    sdtx_printf("tile:   (%d, %d)\n", (int)mouse_tile.x, (int)mouse_tile.y);
    Rect bounds = state.camera->bounds();
    sdtx_printf("camera: (%d, %d, %d, %d)\n", bounds.x, bounds.y, bounds.x + bounds.w, bounds.y + bounds.h);

    if (!state.manager->update(sapp_frame_duration()))
        sapp_quit();
}
