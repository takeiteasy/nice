//
//  camera.c
//  rpg
//
//  Created by George Watson on 24/07/2025.
//

#include "camera.h"

struct camera camera_create(float x, float y, float zoom, float rotation) {
    return (struct camera) {
        .position = Vec2(x, y),
        .zoom = clamp(zoom, .1f, 10.f),
        .rotation = clamp(rotation, 0.f, 360.f),
        .dirty = true,
        .mvp = mat4_identity()
    };
}

void camera_move(struct camera *cam, float dx, float dy) {
    if (dx != 0.f || dy != 0.f)
        cam->dirty = true;
    cam->position += Vec2(dx, dy);
}

void camera_set_position(struct camera *cam, float x, float y) {
    if (cam->position.x != x || cam->position.y != y)
        cam->dirty = true;
    cam->position = Vec2(x, y);
}

void camera_zoom(struct camera *cam, float dz) {
    if (dz != 0.f)
        cam->dirty = true;
    cam->zoom = clamp(cam->zoom + dz, .1f, 10.f);
}

void camera_set_zoom(struct camera *cam, float zoom) {
    if (cam->zoom != zoom)
        cam->dirty = true;
    cam->zoom = clamp(zoom, .1f, 10.f);
}

void camera_rotate(struct camera *cam, float dangle) {
    if (dangle != 0.f)
        cam->dirty = true;
    cam->rotation = clamp(cam->rotation + dangle, 0.f, 360.f);
}

void camera_set_rotation(struct camera *cam, float angle) {
    if (cam->rotation != angle)
        cam->dirty = true;
    cam->rotation = clamp(angle, 0.f, 360.f);
}

void camera_update_mvp(struct camera *cam, int width, int height) {
    mat4 view = ortho(0.f, (float)width, (float)height, 0.f, -1.f, 1.f);
    mat4 world = mat4_translate(-Vec3(cam->position.x, cam->position.y, 0.f)) *
                 mat4_rotate(Vec3(0.f, 0.f, 1.f), to_radians(cam->rotation)) *
                 mat4_scale(Vec3(cam->zoom, cam->zoom, 1.f));
    cam->mvp = view * world;
}
