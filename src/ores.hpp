//
//  ores.hpp
//  rpg
//
//  Created by George Watson on 17/08/2025.
//

#pragma once

#include "global.hpp"
#include "vertex_batch.hpp"
#include "camera.hpp"
#include "entity.hpp"
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
    _COUNT
};

class Ore: Entity<> {
    OreType _type;
    Texture *_texture;

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
        if (type < OreType::Clay || type >= OreType::_COUNT)
            throw std::invalid_argument("Invalid OreType");
        int index = static_cast<int>(type);
        return glm::vec2(0, index == 0 ? 0 : (index - 1) * TILE_ORIGINAL_HEIGHT);
    }

public:
    Ore(OreType type, Texture *texture, glm::vec2 position, int chunk_x, int chunk_y): _type(type), _texture(texture), Entity<>(position, chunk_x, chunk_y) {
        if (type < OreType::Clay || type >= OreType::_COUNT)
            throw std::invalid_argument("Invalid OreType");
    }

    OreType type() const {
        return _type;
    }

    std::string type_string() const {
        return ore_type_to_string(_type);
    }

    std::pair<BasicVertex*, size_t> vertices() override {
        int y = (static_cast<int>(ore_texcoords(_type).y)) / ORE_ORIGINAL_HEIGHT;
        return {generate_quad(_position - (glm::vec2(TILE_WIDTH, TILE_HEIGHT) / 2.f),
                              {ORE_WIDTH, ORE_HEIGHT},
                              {0, y * ORE_ORIGINAL_HEIGHT},
                              {ORE_ORIGINAL_WIDTH, ORE_ORIGINAL_HEIGHT},
                              {_texture->width(), _texture->height()}),
                6};
    }
};

class OreFactory: public EntityFactory<Ore> {
public:
    OreFactory(ChunkManager *cm, Camera *camera, Texture *texture, int chunk_x, int chunk_y)
        : EntityFactory<Ore>(cm, camera, texture, chunk_x, chunk_y) {}

    void add_ores(const std::vector<std::pair<OreType, glm::vec2>>& ores) {
        if (ores.empty())
            return;
        std::lock_guard<std::shared_mutex> lock(_objects_mutex);
        for (const auto& ore : ores)
            _objects.push_back(new Ore(ore.first, _texture, ore.second, _chunk_x, _chunk_y));
        std::cout << fmt::format("Added {} ores to chunk ({}, {})\n", ores.size(), _chunk_x, _chunk_y);
        _dirty.store(true);
    }
};

using OreManagerType = EntityFactoryManager<Ore, OreFactory>;
class OreManager: public OreManagerType {
public:
    OreManager(ChunkManager *manager, Camera *camera, Texture *texture)
        : EntityFactoryManager<Ore, OreFactory>(manager, [manager, camera, texture](uint64_t id) -> OreFactory* {
            auto [x, y] = unindex(id);
            return new OreFactory(manager, camera, texture, x, y);
        }) {}

    void add_ores(uint64_t chunk_id, const std::vector<glm::vec2>& positions) {
        static std::random_device seed;
        static std::mt19937 gen{seed()};
        std::uniform_int_distribution<> ro{1, static_cast<int>(OreType::_COUNT) - 1};

        std::vector<OreType> ore_types;
        ore_types.reserve(positions.size());
        for (int i = 0; i < positions.size(); i++)
            ore_types.push_back(static_cast<OreType>(ro(gen)));

        std::vector<std::pair<OreType, glm::vec2>> ores;
        ores.reserve(positions.size());
        for (size_t i = 0; i < positions.size(); ++i)
            ores.emplace_back(ore_types[i], positions[i]);

        get(chunk_id)->add_ores(ores);
    }

    void build(uint64_t id) override {
        OreManagerType::build(id);
        auto [x, y] = unindex(id);
        std::cout << fmt::format("Ore factory for chunk ({}, {}) finished building\n", x, y);
    }
};
