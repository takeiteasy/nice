//
//  camera.h
//  rpg
//
//  Created by George Watson on 24/07/2025.
//

#pragma once

#include <stdbool.h>
#include "bla.h"

struct camera {
    vec2 position;
    float zoom;
    float rotation;
    bool dirty;
    mat4 mvp;
};

struct camera camera_create(float x, float y, float zoom, float rotation);
void camera_move(struct camera *cam, float dx, float dy);
void camera_set_position(struct camera *cam, float x, float y);
void camera_zoom(struct camera *cam, float dz);
void camera_set_zoom(struct camera *cam, float zoom);
void camera_rotate(struct camera *cam, float dangle);
void camera_set_rotation(struct camera *cam, float angle);
void camera_update_mvp(struct camera *cam, int width, int height);
