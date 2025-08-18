//
//  ores.hh
//  rpg
//
//  Created by George Watson on 17/08/2025.
//

#pragma once

#include "config.h"
#include "asset_manager.hh"
#include "batch.hh"
#include "camera.hh"
#include "game_object.hh"
#include <shared_mutex>
#include <iostream>
#include <vector>
#include "fmt/format.h"
#include "basic.glsl.h"

enum class OreType {
    Clay,
    Bauxite, // Aluminum
    Iron,
    Gold,
    Zinc,
    Quartz, // Silicon/Glass
    Cobalt,
    Nickel,
    Copper,
    COUNT
};

class Ore: GameObject<> {
    OreType _type;

    static inline std::string ore_type_to_string(OreType type) {
        switch (type) {
            case OreType::Clay:
                return "Clay";
            case OreType::Bauxite:
                return "Bauxite";
            case OreType::Iron:
                return "Iron";
            case OreType::Gold:
                return "Gold";
            case OreType::Zinc:
                return "Zinc";
            case OreType::Quartz:
                return "Quartz";
            case OreType::Cobalt:
                return "Cobalt";
            case OreType::Nickel:
                return "Nickel";
            case OreType::Copper:
                return "Copper";
            default:
                throw std::invalid_argument("Unknown OreType");
        }
    }

    static inline glm::vec2 ore_texcoords(OreType type) {
        if (type < OreType::Clay || type >= OreType::COUNT)
            throw std::invalid_argument("Invalid OreType");
        int index = static_cast<int>(type);
        return glm::vec2(0, index == 0 ? 0 : (index - 1) * TILE_ORIGINAL_HEIGHT);
    }

public:
    Ore(OreType type, glm::vec2 position, int chunk_x, int chunk_y, int texture_width, int texture_height): _type(type), GameObject<>(position, chunk_x, chunk_y, texture_width, texture_height) {
        if (type < OreType::Clay || type >= OreType::COUNT)
            throw std::invalid_argument("Invalid OreType");
    }

    OreType type() const {
        return _type;
    }

    std::string type_string() const {
        return ore_type_to_string(_type);
    }

    BasicVertex* vertices() const override {
        int clip_y = static_cast<int>(ore_texcoords(_type).y);
        int tile_index = clip_y / ORE_ORIGINAL_HEIGHT;
        return generate_quad(_position - (glm::vec2(TILE_WIDTH, TILE_HEIGHT) / 2.f),
                             {ORE_WIDTH, ORE_HEIGHT},
                             {0, static_cast<int>(clip_y + (tile_index * ORE_PADDING))},
                             {ORE_ORIGINAL_WIDTH, ORE_ORIGINAL_HEIGHT},
                             {_texture_width, _texture_height});
    }
};

class OreManager: public Factory<Ore> {
public:
    OreManager(Camera *camera, Texture *texture, int chunk_x, int chunk_y): Factory<Ore>(camera, texture, chunk_x, chunk_y) {}

    void add_ores(const std::vector<std::pair<OreType, glm::vec2>>& ores) {
        if (ores.empty())
            return;
        std::lock_guard<std::shared_mutex> lock(_objects_mutex);
        for (const auto& ore : ores)
            _objects.push_back(new Ore(ore.first, ore.second, _chunk_x, _chunk_y, _texture->width(), _texture->height()));
        std::cout << fmt::format("Added {} ores to chunk ({}, {})\n", ores.size(), _chunk_x, _chunk_y);
        _dirty.store(true);
    }
};
