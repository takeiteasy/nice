//
//  robot.hh
//  rpg
//
//  Created by George Watson on 15/08/2025.
//

#pragma once

#include "glm/vec2.hpp"
#include "texture.hh"
#include "camera.hh"
#include "sokol/sokol_time.h"
#include "chunk.hh"
#include "uuid.hh"

struct RobotVertex {
    glm::vec2 position;
    glm::vec2 texcoord;
};

enum class Direction {
    South,
    SouthWest,
    West,
    NorthWest,
    North,
    NorthEast,
    East,
    SouthEast
};

class Robot: public UUID<Robot> {
    int _texture_width, _texture_height;
    glm::vec2 _position;
    glm::vec2 _target;
    Rect _clip = {0, 0, TILE_ORIGINAL_WIDTH, TILE_ORIGINAL_HEIGHT};
    int _frame = 0, _frame_x = 0, _frame_y = 0;
    bool _moving = false;
    Direction _direction = Direction::South;
    uint64_t _last_update_time = 0;
    uint64_t _chunk_id;

    static inline float to_degrees(float radians) {
        return radians * (180.0f / M_PI);
    }

    static inline float to_radians(float degrees) {
        return degrees * (M_PI / 180.0f);
    }

    static inline Direction bearing(glm::vec2 from, glm::vec2 to) {
        float l1 = to_radians(from.x);
        float l2 = to_radians(to.x);
        float dl = to_radians(to.y - from.y);
        return static_cast<Direction>(((((static_cast<int> (to_degrees(std::atan2(std::sin(dl) * std::cos(l2),
                                                                                  std::cos(l1) * std::sin(l2) -
                                                                                  (std::sin(l1) * std::cos(l2) * std::cos(dl))))) + 360) % 360) + 180) % 360) / 45);
    }

public:
    Robot(const glm::vec2& position, uint64_t chunk_id, int texture_width, int texture_height): _chunk_id(chunk_id), _texture_width(texture_width), _texture_height(texture_height) {
        auto [chunk_x, chunk_y] = Chunk::unindex(chunk_id);
        _position = Camera::chunk_tile_to_world({chunk_x, chunk_y}, position);
        _target = _position;
    }

    void update() {
        bool _was_moving = _moving;
        _moving = glm::length(_target - _position) >= .1f;
        if (!_was_moving && _moving) {
            _last_update_time = 0;
            _frame = 0;
            _frame_x = 0;
        }
        if (!_moving) {
            _position = _target;
            _frame_x = 0;
        } else {
            _direction = bearing(_position, _target);
            _frame_y = static_cast<int>(_direction);
            uint64_t current_time = stm_now();
            if (_last_update_time == 0 || stm_diff(current_time, _last_update_time) > stm_sec(1)) {
                _last_update_time = current_time;
                _frame_x = ++_frame % 6;
            }
            _position += glm::normalize(_target - _position) * .1f;
        }
        _clip.x = _frame_x;
        _clip.y = _frame_y;
    }

    RobotVertex* vertices(Camera *camera) const {
        Rect src = {
            static_cast<int>((_clip.x * ROBOT_ORIGINAL_WIDTH) + ((_clip.x + 1) * ROBOT_PADDING)),
            static_cast<int>((_clip.y * ROBOT_ORIGINAL_HEIGHT) + ((_clip.y + 1) * ROBOT_PADDING)),
            ROBOT_ORIGINAL_WIDTH, ROBOT_ORIGINAL_HEIGHT
        };

        glm::vec2 _positions[] = {
            {_position.x, _position.y},                              // Top-left
            {_position.x + ROBOT_WIDTH, _position.y},                // Top-right
            {_position.x + ROBOT_WIDTH, _position.y + ROBOT_HEIGHT}, // Bottom-right
            {_position.x, _position.y + ROBOT_HEIGHT},               // Bottom-left
        };

        float iw = 1.f / _texture_width;
        float ih = 1.f / _texture_height;
        float tl = src.x * iw;
        float tt = src.y * ih;
        float tr = (src.x + src.w) * iw;
        float tb = (src.y + src.h) * ih;
        glm::vec2 _texcoords[4] = {
            {tl, tt}, // top left
            {tr, tt}, // top right
            {tr, tb}, // bottom right
            {tl, tb}, // bottom left
        };

        RobotVertex *v = new RobotVertex[6];
        static uint16_t _indices[] = {0, 1, 2, 2, 3, 0};
        for (int i = 0; i < 6; i++) {
            v[i].position = _positions[_indices[i]];
            v[i].texcoord = _texcoords[_indices[i]];
        }
        return v;
    }

    glm::vec2 position() const { return _position; }
    void set_position(const glm::vec2& position) { _position = position; }
    glm::vec2 target() const { return _target; }
    void set_target(const glm::vec2& target) { _target = target; }
    Rect clip() const { return _clip; }
    void set_clip(const Rect& clip) { _clip = clip; }
};
