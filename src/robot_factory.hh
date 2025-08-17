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
#include "fmt/format.h"

typedef VertexBatch<RobotVertex> RobotVertexBatch;

class RobotFactory {
    std::unordered_map<uint64_t, std::vector<Robot*>> _robot_map;
    mutable std::unordered_map<uint64_t, std::shared_mutex> _robot_mutex_map;
    mutable std::shared_mutex _robot_map_mutex;
    Camera *_camera;
    int _robot_texture_width, _robot_texture_height;

public:
    RobotFactory(Camera *camera, Texture *texture): _camera(camera), _robot_texture_width(texture->width()), _robot_texture_height(texture->height()) {}

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
            _robot_map[chunk_id].push_back(new Robot(pos, chunk_id, _robot_texture_width, _robot_texture_height));
        }
        std::cout << fmt::format("Added {} robots to chunk {}\n", positions.size(), chunk_id);
    }

    void update_robots(uint64_t chunk_id) {
        std::lock_guard<std::shared_mutex> lock(_robot_map_mutex);
        for (auto& robot : _robot_map[chunk_id]) {
            std::shared_lock<std::shared_mutex> lock_chunk(_robot_mutex_map[chunk_id]);
            robot->update();
        }
    }

    void delete_robots(uint64_t chunk_id) {
        std::lock_guard<std::shared_mutex> lock(_robot_map_mutex);
        if (_robot_map.find(chunk_id) == _robot_map.end())
            return;

        std::cout << fmt::format("Deleting {} robot(s) in chunk {}\n", _robot_map[chunk_id].size(), chunk_id);
        
        // Lock the chunk mutex before deleting robots
        if (_robot_mutex_map.find(chunk_id) != _robot_mutex_map.end()) {
            std::lock_guard<std::shared_mutex> lock_chunk(_robot_mutex_map[chunk_id]);
            for (auto& robot : _robot_map[chunk_id])
                delete robot;
            _robot_map.erase(chunk_id);
        } else {
            for (auto& robot : _robot_map[chunk_id])
                delete robot;
            _robot_map.erase(chunk_id);
        }
        
        // Erase the mutex after it's no longer locked
        _robot_mutex_map.erase(chunk_id);
    }

    RobotVertex* vertices(uint64_t chunk_id, size_t *count) {
        std::lock_guard<std::shared_mutex> map_lock(_robot_map_mutex);
        
        // Check if chunk exists in robot map
        if (_robot_map.find(chunk_id) == _robot_map.end() || _robot_map[chunk_id].empty()) {
            if (count)
                *count = 0;
            return nullptr;
        }
        
        // First pass: count valid robots
        size_t valid_robot_count = 0;
        for (Robot* robot : _robot_map[chunk_id])
            if (robot)
                valid_robot_count++;
        
        if (valid_robot_count == 0) {
            if (count)
                *count = 0;
            return nullptr;
        }
        
        size_t _count = valid_robot_count * 6;
        RobotVertex *result = new RobotVertex[_count];
        size_t vertex_index = 0;
        
        for (size_t i = 0; i < _robot_map[chunk_id].size(); ++i) {
            std::lock_guard<std::shared_mutex> lock_map(_robot_mutex_map[chunk_id]);
            Robot *robot = _robot_map[chunk_id][i];
            if (!robot)
                continue;
            RobotVertex *vertices = robot->vertices(_camera);
            std::memcpy(&result[vertex_index], vertices, sizeof(RobotVertex) * 6);
            delete[] vertices;
            vertex_index += 6;
        }
        if (count)
            *count = _count;
        return result;
    }
};
