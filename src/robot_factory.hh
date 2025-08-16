//
//  robot_factory.hh
//  rpg
//
//  Created by George Watson on 15/08/2025.
//

#pragma once

#include "robot.hh"
#include "batch.hh"
#include "glm/vec4.hpp"
#include "robot.glsl.h"

typedef VertexBatch<RobotVertex> RobotVertexBatch;

class RobotFactory {
    std::vector<Robot*> _robots;
    mutable std::shared_mutex _robot_mutex;

    RobotVertexBatch _batch;
    Camera *_camera;
    sg_shader _shader;
    sg_pipeline _pipeline;

public:
    RobotFactory(Camera *camera): _camera(camera) {
        _shader = sg_make_shader(robot_shader_desc(sg_query_backend()));
        sg_pipeline_desc desc = {
            .shader = _shader,
            .layout = {
                .buffers[0].stride = sizeof(RobotVertex),
                .attrs = {
                    [ATTR_robot_position].format = SG_VERTEXFORMAT_FLOAT2,
                    [ATTR_robot_texcoord].format = SG_VERTEXFORMAT_FLOAT2
                }
            },
            .depth = {
                .pixel_format = SG_PIXELFORMAT_DEPTH,
                .compare = SG_COMPAREFUNC_LESS_EQUAL,
                .write_enabled = true
            },
            .cull_mode = SG_CULLMODE_BACK,
            .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8
        };
        _pipeline = sg_make_pipeline(&desc);
        _batch.set_pipeline(_pipeline);
        _batch.set_texture($ASSETS.get<Texture>("robot"));
    }

    ~RobotFactory() {
        std::lock_guard<std::shared_mutex> lock(_robot_mutex);
        for (auto& robot : _robots)
            delete robot;
        if (sg_query_shader_state(_shader) == SG_RESOURCESTATE_VALID)
            sg_destroy_shader(_shader);
        if (sg_query_pipeline_state(_pipeline) == SG_RESOURCESTATE_VALID)
            sg_destroy_pipeline(_pipeline);
    }

    void add_robots(std::vector<glm::vec2> positions) {
        std::lock_guard<std::shared_mutex> lock(_robot_mutex);
        for (const auto& pos : positions)
            _robots.push_back(new Robot(pos));
    }

    void update_robots() {
        std::lock_guard<std::shared_mutex> lock(_robot_mutex);
        for (const auto& robot : _robots) {
            // TODO: Move robot about
        }
    }

    void draw_robots(Camera *camera) const {
        std::lock_guard<std::shared_mutex> lock(_robot_mutex);
        for (const auto& robot : _robots) {
            // TODO: Draw robots
        }
    }
};
