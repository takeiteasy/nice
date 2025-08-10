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
        return (x < other.x + other.w) &&
               (x + w > other.x) &&
               (y < other.y + other.h) &&
               (y + h > other.y);
    }
};

class Camera {
    glm::vec2 _position = glm::vec2(0.f);
    float _zoom = 1.f;
    float _rotation = 0.f;

    Rect _bounds(float zoom) {
        float visible_width = framebuffer_width() / zoom;
        float visible_height = framebuffer_height() / zoom;
        float left = _position.x - (visible_width * .5f);
        float top = _position.y - (visible_height * .5f);
        return (struct Rect) {
            .x = (int)left,
            .y = (int)top,
            .w = (int)visible_width,
            .h = (int)visible_height
        };
    }

    template<typename T> T _clamp(T value) {
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
        _zoom = _clamp(z);
        dirty = true;
    }

    void zoom_by(float z) {
        _zoom = _clamp(_zoom + z);
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
        glm::mat4 projection = glm::ortho(0.f, (float)w, (float)h, 0.f, -1.f, 1.f);
        glm::mat4 view = glm::mat4(1.f);
        float hw = (float)w / 2.f;
        float hh = (float)h / 2.f;
        // Apply camera transformations: translate to center, scale for zoom, rotate, then translate to position
        view = glm::translate(view, glm::vec3(hw, hh, 0.f));
        view = glm::scale(view, glm::vec3(_zoom, _zoom, 1.f));
        view = glm::rotate(view, glm::radians(_rotation), glm::vec3(0.f, 0.f, 1.f));
        view = glm::translate(view, glm::vec3(-_position, 0.f));
        view = glm::translate(view, glm::vec3(-hw, -hh, 0.f));
        return projection * view;
    }

    glm::vec2 world_to_screen(glm::vec2 world_pos) const {
        int w = framebuffer_width();
        int h = framebuffer_height();

        // Apply camera transformation: translate relative to camera, then zoom, then center on screen
        glm::vec2 relative_pos = world_pos - _position;
        glm::vec2 zoomed_pos = relative_pos * _zoom;
        glm::vec2 screen_pos = zoomed_pos + glm::vec2(w * .5f, h * .5f);

        // Convert from framebuffer coordinates to actual screen coordinates
        return screen_pos * glm::vec2((float)sapp_width() / w, (float)sapp_height() / h);
    }

    glm::vec2 screen_to_world(glm::vec2 screen_pos) const {
        int w = framebuffer_width();
        int h = framebuffer_height();

        // Convert from screen coordinates to framebuffer coordinates
        glm::vec2 fb_pos = screen_pos * glm::vec2((float)w / sapp_width(), (float)h / sapp_height());

        // Reverse the camera transformation: uncenter, unzoom, then translate back to world
        glm::vec2 centered_pos = fb_pos - glm::vec2(w * .5f, h * .5f);
        glm::vec2 unzoomed_pos = centered_pos / _zoom;
        glm::vec2 world_pos = unzoomed_pos + _position;

        return world_pos;
    }

    glm::vec2 world_to_tile(glm::vec2 world) const {
        // Convert world position to tile coordinates within a chunk
        // First get the chunk coordinates
        glm::vec2 chunk_pos = world_to_chunk(world);

        // Calculate the world position of the chunk's top-left corner
        float chunk_world_x = chunk_pos.x * CHUNK_WIDTH * TILE_WIDTH;
        float chunk_world_y = chunk_pos.y * CHUNK_HEIGHT * TILE_HEIGHT;

        // Get the relative position within the chunk
        float rel_x = world.x - chunk_world_x;
        float rel_y = world.y - chunk_world_y;

        // Convert to tile coordinates within the chunk
        int tile_x = (int)floor(rel_x / TILE_WIDTH);
        int tile_y = (int)floor(rel_y / TILE_HEIGHT);

        // Clamp to chunk bounds and handle negative coordinates
        return glm::vec2(tile_x < 0 ? 0 : (tile_x >= CHUNK_WIDTH ? CHUNK_WIDTH - 1 : tile_x),
                         tile_y < 0 ? 0 : (tile_y >= CHUNK_HEIGHT ? CHUNK_HEIGHT - 1 : tile_y));
    }

    glm::vec2 world_to_chunk(glm::vec2 world) const {
        // Calculate chunk size in world units
        float chunk_world_width = CHUNK_WIDTH * TILE_WIDTH;
        float chunk_world_height = CHUNK_HEIGHT * TILE_HEIGHT;

        // Convert world position to chunk coordinates
        return glm::vec2(floor(world.x / chunk_world_width),
                         floor(world.y / chunk_world_height));
    }

    struct Rect bounds() {
        return _bounds(_zoom);
    }

    struct Rect max_bounds() {
        return _bounds(std::max(MIN_ZOOM - .1f, __FLT_EPSILON__));
    }
};
