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

struct OreVertex {
    glm::vec2 position;
    glm::vec2 texcoord;
};

class Ore: public UUID<Ore> {
    int _texture_width, _texture_height;
    glm::vec2 _position;
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
    Ore(OreType type, glm::vec2 position, int chunk_x, int chunk_y, int texture_width, int texture_height): _type(type), _position(Camera::chunk_tile_to_world({chunk_x, chunk_y}, position)), _texture_width(texture_width), _texture_height(texture_height) {
        if (type < OreType::Clay || type >= OreType::COUNT)
            throw std::invalid_argument("Invalid OreType");
    }

    OreType type() const {
        return _type;
    }

    std::string type_string() const {
        return ore_type_to_string(_type);
    }

    OreVertex* vertices() const {
        int _width = ORE_WIDTH - (TILE_WIDTH / 2);
        int _height = ORE_HEIGHT - (TILE_HEIGHT / 2);
        glm::vec2 _positions[] = {
            {_position.x, _position.y},                    // Top-left
            {_position.x + _width, _position.y},           // Top-right
            {_position.x + _width, _position.y + _height}, // Bottom-right
            {_position.x, _position.y + _height},          // Bottom-left
        };

        int clip_y = static_cast<int>(ore_texcoords(_type).y);
        int tile_index = clip_y / ORE_ORIGINAL_HEIGHT;
        Rect src = {
            0, static_cast<int>(clip_y + (tile_index * ORE_PADDING)),
            ORE_ORIGINAL_WIDTH, ORE_ORIGINAL_HEIGHT
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

        OreVertex *v = new OreVertex[6];
        static uint16_t _indices[] = {0, 1, 2, 2, 3, 0};
        for (int i = 0; i < 6; i++) {
            v[i].position = _positions[_indices[i]];
            v[i].texcoord = _texcoords[_indices[i]];
        }
        return v;
    }
};
