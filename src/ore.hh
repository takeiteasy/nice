//
//  ore.hh
//  rpg
//
//  Created by George Watson on 17/08/2025.
//

#pragma once

#include "config.h"
#include "uuid.hh"
#include "camera.hh"

enum class OreType {
    Bauxite, // Aluminum
    Iron,
    Gold,
    Zinc,
    Quartz, // Silicon/Glass
    Cobalt,
    Nickel,
    Copper
};

class Ore: public UUID<Ore> {
    glm::vec2 _position;
    int _chunk_x, _chunk_y;
    OreType _type;

    static inline std::string ore_type_to_string(OreType type) {
        switch (type) {
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
        }
    }

    static inline glm::vec2 ore_texcoords(OreType type) {
        int index = static_cast<int>(type);
        return glm::vec2(0, index * TILE_ORIGINAL_HEIGHT);
    }

public:
    Ore(OreType type, glm::vec2 position, int chunk_x, int chunk_y): _type(type), _position(position), _chunk_x(chunk_x), _chunk_y(chunk_y) {}

    OreType type() const {
        return _type;
    }

    std::string type_string() const {
        return ore_type_to_string(_type);
    }

    glm::vec2 texcoords() const {
        return ore_texcoords(_type);
    }

    glm::vec2 position() const {
        return Camera::chunk_tile_to_world({_chunk_x, _chunk_y}, _position);
    }
};
