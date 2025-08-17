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

// TODO: Move RobotVertexBatch to chunk_factory
//       collect vertices from robot_factory->draw
//       and draw singular batch for all robots

typedef VertexBatch<RobotVertex> RobotVertexBatch;

class RobotFactory {
    std::unordered_map<uint64_t, std::vector<Robot*>> _robot_map;
    mutable std::unordered_map<uint64_t, std::shared_mutex> _robot_mutex_map;
    mutable std::shared_mutex _robot_map_mutex;
    Camera *_camera;

public:
    RobotFactory(Camera *camera): _camera(camera) {}

    ~RobotFactory() {
        std::lock_guard<std::shared_mutex> lock(_robot_map_mutex);
        for (auto& pair : _robot_map) {
            uint64_t chunk_id = pair.first;
            std::lock_guard<std::shared_mutex> lock(_robot_mutex_map[chunk_id]);
            std::vector<Robot*> &robots = pair.second;
            for (auto& robot : robots)
                delete robot;
        }
    }

    void add_robots(uint64_t chunk_id, std::vector<glm::vec2> positions) {
        std::lock_guard<std::shared_mutex> lock(_robot_map_mutex);
        for (const auto& pos : positions) {
            std::lock_guard<std::shared_mutex> lock(_robot_mutex_map[chunk_id]);
            _robot_map[chunk_id].push_back(new Robot(pos));
        }
    }

    void update_robots(uint64_t chunk_id) {
        std::lock_guard<std::shared_mutex> lock(_robot_map_mutex);
        for (auto& robot : _robot_map[chunk_id]) {
            std::shared_lock<std::shared_mutex> lock_chunk(_robot_mutex_map[chunk_id]);
            robot->update();
        }
    }

    RobotVertex* vertices(uint64_t chunk_id, size_t *count) {
        std::lock_guard<std::shared_mutex> map_lock(_robot_map_mutex);
        size_t _count = _robot_map[chunk_id].size() * 6;
        RobotVertex *result = new RobotVertex[_count];
        for (size_t i = 0; i < _robot_map[chunk_id].size(); ++i) {
            std::lock_guard<std::shared_mutex> lock_map(_robot_mutex_map[chunk_id]);
            Robot *robot = _robot_map[chunk_id][i];
            RobotVertex *vertices = robot->vertices(_camera);
            std::memcpy(&result[i * 6], vertices, sizeof(RobotVertex) * 6);
            delete[] vertices;
        }
        if (count)
            *count = _count;
        return result;
    }
};
