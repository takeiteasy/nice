//
//  camera.h
//  rpg
//
//  Created by George Watson on 24/07/2025.
//

#pragma once

#include "HandmadeMath.h"
#include <stdbool.h>

struct camera {
    HMM_Vec2 position;
    float zoom;
    float rotation;
    bool dirty;
};

struct rect {
    int X, Y, W, H;
};

#ifndef MAX_ZOOM
#define MAX_ZOOM 10.f
#endif
#ifndef MIN_ZOOM
#define MIN_ZOOM .1f
#endif

struct camera camera_create(float x, float y, float zoom, float rotation);
void camera_move(struct camera *cam, float dx, float dy);
void camera_set_position(struct camera *cam, float x, float y);
void camera_zoom(struct camera *cam, float dz);
void camera_set_zoom(struct camera *cam, float zoom);
void camera_rotate(struct camera *cam, float dangle);
void camera_set_rotation(struct camera *cam, float angle);
HMM_Mat4 camera_mvp(struct camera *cam, int width, int height);

HMM_Vec2 camera_world_to_screen(struct camera *cam, HMM_Vec2 world_pos);
HMM_Vec2 camera_screen_to_world(struct camera *cam, HMM_Vec2 screen_pos);

struct rect camera_bounds_ex(float x, float y, float zoom);
struct rect camera_bounds(struct camera *cam);
