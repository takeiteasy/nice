//
//  camera.c
//  rpg
//
//  Created by George Watson on 24/07/2025.
//

#include "camera.h"

#define clamp(value, minval, maxval)       \
    ({__typeof__(value) _value  = (value);    \
      __typeof__(value) _minval = (minval);   \
      __typeof__(value) _maxval = (maxval);   \
      _value < _minval ? _minval : (_value > _maxval ? _maxval : _value); })

struct camera camera_create(float x, float y, float zoom, float rotation) {
    return (struct camera) {
        .position = HMM_V2(x, y),
        .zoom = clamp(zoom, .1f, 10.f),
        .rotation = clamp(rotation, 0.f, 360.f),
        .dirty = true
    };
}

void camera_move(struct camera *cam, float dx, float dy) {
    if (dx != 0.f || dy != 0.f)
        cam->dirty = true;
    cam->position = HMM_AddV2(cam->position, HMM_V2(-dx, -dy));
}

void camera_set_position(struct camera *cam, float x, float y) {
    if (cam->position.X != x || cam->position.Y != y)
        cam->dirty = true;
    cam->position = HMM_V2(x, y);
}

void camera_zoom(struct camera *cam, float dz) {
    if (dz != 0.f)
        cam->dirty = true;
    cam->zoom = clamp(cam->zoom + dz, MIN_ZOOM, MAX_ZOOM);
}

void camera_set_zoom(struct camera *cam, float zoom) {
    if (cam->zoom != zoom)
        cam->dirty = true;
    cam->zoom = clamp(zoom, MIN_ZOOM, MAX_ZOOM);
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

HMM_Mat4 camera_mvp(struct camera *cam, int width, int height) {
    HMM_Mat4 projection = HMM_Orthographic_LH_NO(0, (float)width, (float)height, 0, -1, 1);
    HMM_Mat4 view = HMM_M4D(1.f);
    float hw = (float)width / 2.f;
    float hh = (float)height / 2.f;
    view = HMM_Translate(HMM_V3(hw, hh, 0));
    view = HMM_MulM4(view, HMM_Scale(HMM_V3(cam->zoom, cam->zoom, 1)));
    view = HMM_MulM4(view, HMM_Rotate_LH(-cam->rotation, HMM_V3(0, 0, 1)));
    view = HMM_MulM4(view, HMM_Translate(HMM_V3(-cam->position.X, -cam->position.Y, 0)));
    view = HMM_MulM4(view, HMM_Translate(HMM_V3(-hw, -hh, 0)));
    return HMM_MulM4(projection, view);
}

HMM_Vec2 camera_world_to_screen(struct camera *cam, HMM_Vec2 world_pos) {
    int fb_width = framebuffer_width();
    int fb_height = framebuffer_height();
    // First convert to framebuffer coordinates
    HMM_Vec2 fb_pos = HMM_V2(
        (world_pos.X - cam->position.X) * cam->zoom + fb_width * 0.5f,
        (world_pos.Y - cam->position.Y) * cam->zoom + fb_height * 0.5f
    );

    // Scale from framebuffer to window coordinates
    return HMM_V2(
        fb_pos.X * ((float)sapp_width() / fb_width),
        fb_pos.Y * ((float)sapp_height() / fb_height)
    );
}

HMM_Vec2 camera_screen_to_world(struct camera *cam, HMM_Vec2 screen_pos) {
    int fb_width = framebuffer_width();
    int fb_height = framebuffer_height();
    // Scale from window coordinates to framebuffer coordinates
    HMM_Vec2 fb_pos = HMM_V2(
        screen_pos.X * ((float)fb_width / sapp_width()),
        screen_pos.Y * ((float)fb_height / sapp_height())
    );

    // Convert framebuffer coordinates to world coordinates
    return HMM_V2(
        (fb_pos.X - fb_width * 0.5f) / cam->zoom + cam->position.X,
        (fb_pos.Y - fb_height * 0.5f) / cam->zoom + cam->position.Y
    );
}

struct rect camera_bounds_ex(float x, float y, float zoom) {
    int fb_width = framebuffer_width();
    int fb_height = framebuffer_height();
    // Scale ratio between framebuffer and window
    float width_ratio = (float)fb_width / sapp_width();
    float height_ratio = (float)fb_height / sapp_height();

    // Calculate visible area in world coordinates based on framebuffer dimensions
    float visible_width = fb_width / zoom;
    float visible_height = fb_height / zoom;

    float left = x - (visible_width * 0.5f);
    float top = y - (visible_height * 0.5f);

    return (struct rect) {
        .X = (int)left,
        .Y = (int)top,
        .W = (int)visible_width,
        .H = (int)visible_height
    };
}

struct rect camera_bounds(struct camera *cam) {
    return camera_bounds_ex(cam->position.X, cam->position.Y, cam->zoom);
}
