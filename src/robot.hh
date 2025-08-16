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

struct RobotVertex {
    glm::vec2 position;
    glm::vec2 texcoord;
};

class Robot {
    int _texture_width, _texture_height;
    glm::vec2 _position;
    glm::vec2 _target;
    glm::vec4 _color = glm::vec4(1.f, 1.f, 1.f, 1.f);
    Rect _clip = {0, 0, TILE_ORIGINAL_WIDTH, TILE_ORIGINAL_HEIGHT};

public:
    Robot(const glm::vec2& position): _position(position), _target(position) {
        Texture *texture = $Assets.get<Texture>("robot");
        _texture_width = texture->width();
        _texture_height = texture->height();
    }

    void update() {
        if (glm::length(_target - _position) < .1f)
            _position = _target;
        else
            _position += glm::normalize(_target - _position) * .1f;
    }

    RobotVertex* draw(Camera *camera) const {
        if (camera->bounds().contains(_position)) // change to bounds
            return nullptr;

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
    glm::vec4 color() const { return _color; }
    void set_color(const glm::vec4& color) { _color = color; }
    Rect clip() const { return _clip; }
    void set_clip(const Rect& clip) { _clip = clip; }
};
