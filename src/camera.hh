//
//  camera.hh
//  rpg
//
//  Created by George Watson on 03/08/2025.
//

#pragma once

#include "config.h"
#include "glm/vec2.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "sokol/sokol_app.h"
#include "framebuffer.hh"

struct Rect {
    int x, y, w, h;

    bool contains(glm::vec2 point) const {
        return point.x >= x && point.x <= x + w && point.y >= y && point.y <= y + h;
    }

    bool intersects(const Rect &other) const {
        return !(x + w < other.x || x > other.x + other.w || y + h < other.y || y > other.y + other.h);
    }
};

class Camera {
    glm::vec2 _position = glm::vec2(0.f);
    float _zoom = 1.f;
    float _rotation = 0.f;

    Rect _bounds(float zoom) {
        float visible_width = framebuffer_width() / zoom;
        float visible_height = framebuffer_height() / zoom;
        float left = _position.x - (visible_width * 0.5f);
        float top = _position.y - (visible_height * 0.5f);
        return (struct Rect) {
            .x = (int)left,
            .y = (int)top,
            .w = (int)visible_width,
            .h = (int)visible_height
        };
    }

    template<typename T> T clamp(T value) {
        return (value < MIN_ZOOM) ? MIN_ZOOM : (value > MAX_ZOOM) ? MAX_ZOOM : value;
    }

public:
    bool dirty = true;

    Camera(): _position(0.f), _zoom(1.f), _rotation(0.f), dirty(true) {}
    Camera(glm::vec2 _position, float _zoom = 1.f, float _rotation = 0.f): _position(_position), _zoom(_zoom), _rotation(_rotation), dirty(true) {}

    void set_position(glm::vec2 pos) {
        _position = pos;
        dirty = true;
    }

    void move_by(glm::vec2 offset) {
        _position += offset;
        dirty = true;
    }

    glm::vec2 position() const {
        return _position;
    }

    void set_zoom(float z) {
        _zoom = clamp(z);
        dirty = true;
    }

    void zoom_by(float z) {
        _zoom = clamp(_zoom + z);
        dirty = true;
    }

    float zoom() const {
        return _zoom;
    }

    void set_rotation(float r) {
        _rotation = r;
        dirty = true;
    }

    void rotate_by(float r) {
        _rotation += r;
        dirty = true;
    }

    float rotation() const {
        return _rotation;
    }

    glm::mat4 matrix() const {
        int w = framebuffer_width();
        int h = framebuffer_height();
        float hw = (float)w / 2.f;
        float hh = (float)h / 2.f;
        glm::mat4 projection = glm::ortho(0.f, (float)w, (float)h, 0.f, -1.f, 1.f);
        glm::mat4 view = glm::mat4(1.f);
        view = glm::translate(view, glm::vec3(hw, hh, 0.f)); // Center the view
        view = glm::translate(view, glm::vec3(-_position, 0.f));
        view = glm::rotate(view, glm::radians(_rotation), glm::vec3(0.f, 0.f, 1.f));
        view = glm::scale(view, glm::vec3(_zoom, _zoom, 1.f));
        view = glm::translate(view, glm::vec3(-hw, -hh, 0.f)); // Adjust for center
        return projection * view;
    }

    glm::vec2 world_to_screen(glm::vec2 world_pos) const {
        int w = framebuffer_width();
        int h = framebuffer_height();
        return glm::vec2((world_pos.x - _position.x) * _zoom + w * .5f,
                         (world_pos.y - _position.y) * _zoom + h * .5f) *
               glm::vec2((float)sapp_width() / w,
                         (float)sapp_height() / h);
    }

    glm::vec2 screen_to_world(glm::vec2 screen_pos) const {
        int w = framebuffer_width();
        int h = framebuffer_height();
        return glm::vec2(screen_pos.x * ((float)w / sapp_width()),
                         screen_pos.y * ((float)h / sapp_height())) -
                glm::vec2(w * .5f, h * .5f) / _zoom + _position;
    }

    glm::vec2 world_to_tile(glm::vec2 world) const {
        const static glm::vec2 SIZE = glm::vec2(CHUNK_WIDTH * TILE_WIDTH, CHUNK_HEIGHT * TILE_HEIGHT);
        glm::vec2 final = glm::vec2((int)((world.x / SIZE.x) * CHUNK_WIDTH  + (fmod(world.x, SIZE.x) / TILE_WIDTH)) % CHUNK_WIDTH,
                                    (int)((world.y / SIZE.y) * CHUNK_HEIGHT + (fmod(world.y, SIZE.y) / TILE_HEIGHT)) % CHUNK_HEIGHT);
        return glm::vec2(final.x < 0 ? CHUNK_WIDTH + final.x : final.x,
                         final.y < 0 ? CHUNK_HEIGHT + final.y : final.y);
    }

    glm::vec2 world_to_chunk(glm::vec2 world) {
        const static glm::vec2 SIZE = glm::vec2(CHUNK_WIDTH * TILE_WIDTH, CHUNK_HEIGHT * TILE_HEIGHT);
        return glm::vec2(floor(world.x / SIZE.x),
                         floor(world.y / SIZE.y));
    }

    struct Rect bounds() {
        return _bounds(_zoom);
    }

    struct Rect max_bounds() {
        return _bounds(MIN_ZOOM);
    }
};
