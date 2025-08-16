//
//  robot_factory.hh
//  rpg
//
//  Created by George Watson on 15/08/2025.
//

#pragma once

#include "robot.hh"
#include "batch.hh"
#include "jobs.hh"
#include "glm/vec4.hpp"
#include "basic.glsl.h"

typedef VertexBatch<RobotVertex> RobotVertexBatch;

class RobotFactory {
    std::vector<Robot*> _robots;
    mutable std::shared_mutex _robot_mutex;
    JobPool _job_pool = JobPool(std::thread::hardware_concurrency());

    RobotVertexBatch _batch;
    Camera *_camera;

public:
    RobotFactory(Camera *camera): _camera(camera) {
        _batch.set_texture($Assets.get<Texture>("robot"));
    }

    ~RobotFactory() {
        std::lock_guard<std::shared_mutex> lock(_robot_mutex);
        for (auto& robot : _robots)
            delete robot;
    }

    void add_robots(std::vector<glm::vec2> positions) {
        std::lock_guard<std::shared_mutex> lock(_robot_mutex);
        for (const auto& pos : positions)
            _robots.push_back(new Robot(pos));
    }

    void update_robots() {
        std::lock_guard<std::shared_mutex> lock(_robot_mutex);
        for (auto& robot : _robots)
            robot->update();
    }

    void draw_robots(const glm::mat4& mvp) {
        std::lock_guard<std::shared_mutex> lock(_robot_mutex);
    }
};
