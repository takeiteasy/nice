//
//  ore_manager.hh
//  rpg
//
//  Created by George Watson on 17/08/2025.
//

#pragma once

#include "assets.hh"
#include "ore.hh"
#include "batch.hh"
#include <shared_mutex>
#include <iostream>
#include "fmt/format.h"
#include "basic.glsl.h"

// TODO: Create generic virtual Manager, Factory and Component class
// TODO: Rewrite OreManager, OreFactory and Ore classes to use the generic classes

typedef VertexBatch<OreVertex> OreVertexBatch;

class OreManager {
    int _chunk_x, _chunk_y;
    int _texture_width, _texture_height;
    Camera *_camera;
    std::vector<Ore*> _ores;
    mutable std::shared_mutex _ores_mutex;
    OreVertexBatch _batch;
    mutable std::shared_mutex _batch_mutex;
    std::atomic<bool> _dirty{false};

public:
    OreManager(Camera *camera, int chunk_x, int chunk_y): _camera(camera), _chunk_x(chunk_x), _chunk_y(chunk_y) {
        auto ore_texture = $Assets.get<Texture, true>("assets/ores.exploded.png");
        if (!ore_texture.has_value())
            throw std::runtime_error("Failed to load ore texture");
        _texture_width = ore_texture.value()->width();
        _texture_height = ore_texture.value()->height();
        _batch.set_texture(ore_texture.value());
    }

    void add_ores(const std::vector<std::pair<OreType, glm::vec2>>& ores) {
        if (ores.empty())
            return;
        std::lock_guard<std::shared_mutex> lock(_ores_mutex);
        for (const auto& ore : ores)
            _ores.push_back(new Ore(ore.first, ore.second, _chunk_x, _chunk_y, _texture_width, _texture_height));
        std::cout << fmt::format("Added {} ores to chunk ({}, {})\n", ores.size(), _chunk_x, _chunk_y);
        _dirty.store(true);
    }

    // TODO: Check if ore is visible before adding to batch
    // TODO: Run this in a separate thread
    void build() {
        if (!_dirty.load())
            return;

        std::lock_guard<std::shared_mutex> lock(_batch_mutex);
        _batch.clear();
        _batch.reserve(_ores.size() * 6);
        for (const auto& ore : _ores) {
            OreVertex *vertices = ore->vertices();
            _batch.add_vertices(vertices, 6);
            delete[] vertices;
        }
        _batch.build();
        _dirty.store(false);

        std::cout << fmt::format("Built ore batch ({}) for chunk ({}, {})\n", _ores.size(), _chunk_x, _chunk_y);
    }

    void draw() {
        std::shared_lock<std::shared_mutex> lock(_batch_mutex);
        if (_batch.empty() || !_batch.is_ready())
            return;
        vs_params_t vs_params = { .mvp = _camera->matrix() };
        sg_range params = SG_RANGE(vs_params);
        sg_apply_uniforms(UB_vs_params, &params);
        _batch.flush();
    }

    void mark_dirty() {
        _dirty.store(true);
    }

    bool is_dirty() const {
        return _dirty.load();
    }
};
