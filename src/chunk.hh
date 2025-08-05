//
//  chunk.hh
//  rpg
//
//  Created by George Watson on 03/08/2025.
//

#pragma once

#include "config.h"
#include <stdint.h>
#include <mutex>
#include <atomic>
#include "camera.hh"
#include "texture.hh"
#include "glm/vec2.hpp"
#include "glm/mat4x4.hpp"
#include "default.glsl.h"
#include <random>

union Tile {
    struct {
        uint8_t bitmask;
        uint8_t visible;
        uint8_t solid;
        uint8_t extra;
    };
    uint32_t value;
};

enum class ChunkState {
    CHUNK_STATE_DORMANT,
    CHUNK_STATE_ACTIVE,
    CHUNK_STATE_UNLOAD
};

static inline const char* chunk_state_to_string(ChunkState state) {
    switch (state) {
        case ChunkState::CHUNK_STATE_DORMANT:
            return "DORMANT";
        case ChunkState::CHUNK_STATE_ACTIVE:
            return "ACTIVE";
        case ChunkState::CHUNK_STATE_UNLOAD:
            return "UNLOAD";
        default:
            assert(0);
    }
}

struct ChunkVertex {
    glm::vec2 position;
    glm::vec2 texcoord;
};

static const glm::vec2 Autotile3x3Simplified[256] = {
    [0] = {0, 3},
    [2] = {0, 2},
    [8] = {3, 3},
    [10] = {3, 2},
    [11] = {11, 3},
    [16] = {1, 3},
    [18] = {1, 2},
    [22] = {8, 3},
    [24] = {2, 3},
    [26] = {2, 2},
    [27] = {6, 3},
    [30] = {5, 3},
    [31] = {9, 3},
    [64] = {0, 0},
    [66] = {0, 1},
    [72] = {3, 0},
    [74] = {3, 1},
    [75] = {7, 2},
    [80] = {1, 0},
    [82] = {1, 1},
    [86] = {4, 2},
    [88] = {2, 0},
    [90] = {2, 1},
    [91] = {4, 0},
    [94] = {7, 0},
    [95] = {10, 3},
    [104] = {11, 0},
    [106] = {7, 1},
    [107] = {11, 2},
    [120] = {6, 0},
    [122] = {4, 3},
    [123] = {11, 1},
    [126] = {9, 1},
    [127] = {6, 2},
    [208] = {8, 0},
    [210] = {4, 1},
    [214] = {8, 1},
    [216] = {5, 0},
    [218] = {7, 3},
    [219] = {10, 2},
    [222] = {8, 2},
    [223] = {5, 2},
    [248] = {10, 0},
    [250] = {9, 0},
    [251] = {6, 1},
    [254] = {5, 1},
    [255] = {9, 2},
};

class Chunk {
    int _x, _y;
    Tile _tiles[CHUNK_WIDTH][CHUNK_HEIGHT];
    const Texture *_tilemap;
    std::atomic<ChunkState> _state;
    sg_bindings _bind;
    glm::mat4 _mvp;

    mutable std::mutex _chunk_mutex;

    static int _max(int a, int b) {
        return (a > b) ? a : b;
    }

    void build() {
        std::lock_guard<std::mutex> lock(_chunk_mutex);

        float hw = framebuffer_width() / 2.f;
        float hh = framebuffer_height() / 2.f;
        ChunkVertex vertices[CHUNK_SIZE * 6];
        memset(vertices, 0, sizeof(vertices));
        for (int x = 0; x < CHUNK_WIDTH; x++)
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                Tile *tile = &_tiles[x][y];
                if (!tile->solid)
                    continue;

                glm::vec2 clip = Autotile3x3Simplified[tile->bitmask];
                Rect src = {
                    static_cast<int>((clip.x * TILE_ORIGINAL_WIDTH) + ((clip.x + 1) * TILE_PADDING)),
                    static_cast<int>((clip.y * TILE_ORIGINAL_HEIGHT) + ((clip.y + 1) * TILE_PADDING)),
                    TILE_ORIGINAL_WIDTH, TILE_ORIGINAL_HEIGHT
                };

                float hqw = TILE_WIDTH / 2.f;
                float hqh = TILE_HEIGHT / 2.f;
                glm::vec2 _positions[] = {
                    {hw - hqw, hh - hqh}, // Top-left
                    {hw + hqw, hh - hqh}, // Top-right
                    {hw + hqw, hh + hqh}, // Bottom-right
                    {hw - hqw, hh + hqh}, // Bottom-left
                };

                float iw = 1.f / _tilemap->width();
                float ih = 1.f / _tilemap->height();
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

                uint16_t indices[] = { 0, 1, 2, 2, 3, 0 };
                for (int i = 0; i < 6; i++) {
                    ChunkVertex *v = &vertices[(y * CHUNK_WIDTH + x) * 6 + i];
                    glm::vec2 offset = glm::vec2(x * TILE_WIDTH, y * TILE_HEIGHT);
                    v->position = _positions[indices[i]] + offset;
                    v->texcoord = _texcoords[indices[i]];
                }
            }

        dirty.store(false);
        sg_buffer_desc desc = {
            .data = {
                .ptr = vertices,
                .size = sizeof(vertices)
            }
        };
        _bind.vertex_buffers[0] = sg_make_buffer(&desc);
    }

    static void cellular_automata(unsigned int width, unsigned int height, unsigned int fill_chance, unsigned int smooth_iterations, unsigned int survive, unsigned int starve, uint8_t* result) {
        memset(result, 0, width * height * sizeof(uint8_t));
        // Randomly fill the grid
        std::random_device seed;
        std::mt19937 gen{seed()};
        std::uniform_int_distribution<> ud{1, 100};
        for (int x = 0; x < width; x++)
            for (int y = 0; y < height; y++)
                result[y * width + x] = ud(gen) < fill_chance;
        // Run cellular-automata on grid n times
        for (int i = 0; i < _max(smooth_iterations, 1); i++)
            for (int x = 0; x < width; x++)
                for (int y = 0; y < height; y++) {
                    // Count the cell's living neighbours
                    int neighbours = 0;
                    for (int nx = x - 1; nx <= x + 1; nx++)
                        for (int ny = y - 1; ny <= y + 1; ny++)
                            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                                if ((nx != x || ny != y) && result[ny * width + nx] > 0)
                                    neighbours++;
                            } else
                                neighbours++;
                    // Update cell based on neighbour and surive/starve values
                    if (neighbours > survive)
                        result[y * width + x] = 1;
                    else if (neighbours < starve)
                        result[y * width + x] = 0;
                }
    }

    static uint8_t tile_bitmask(Chunk *chunk, int cx, int cy, int oob) {
        uint8_t neighbours[9] = {0};
        for (int x = -1; x < 2; x++)
            for (int y = -1; y < 2; y++) {
                int dx = cx+x, dy = cy+y;
                // Direct access to _tiles since we're already holding the mutex in fill()
                Tile* tile = dx < 0 || dy < 0 || dx >= CHUNK_WIDTH || dy >= CHUNK_HEIGHT ? nullptr : &chunk->_tiles[dx][dy];
                neighbours[(y+1)*3+(x+1)] = !x && !y ? 0 : tile == nullptr ? oob : tile->solid ? 1 : 0;
            }
        neighbours[0] = !neighbours[1] || !neighbours[3] ? 0 : neighbours[0];
        neighbours[2] = !neighbours[1] || !neighbours[5] ? 0 : neighbours[2];
        neighbours[6] = !neighbours[7] || !neighbours[3] ? 0 : neighbours[6];
        neighbours[8] = !neighbours[7] || !neighbours[5] ? 0 : neighbours[8];
        uint8_t result = 0;
        for (int y = 0, n = 0; y < 3; y++)
            for (int x = 0; x < 3; x++)
                if (!(y == 1 && x == 1))
                    result += (neighbours[y * 3 + x] << n++);
        return result;
    }

public:
    std::atomic<bool> dirty{true};  // Make dirty flag atomic

    Chunk(int x, int y, const Texture* texture): _x(x), _y(y), dirty(true) {
        memset(_tiles, 0, sizeof(_tiles));
        memset(&_bind, 0, sizeof(sg_bindings));
        _tilemap = texture;
        _tilemap->bind(_bind);
        _state.store(ChunkState::CHUNK_STATE_ACTIVE);
    }

    ~Chunk() {
        std::lock_guard<std::mutex> lock(_chunk_mutex);
        if (sg_query_buffer_state(_bind.vertex_buffers[0]) == SG_RESOURCESTATE_VALID)
            sg_destroy_buffer(_bind.vertex_buffers[0]);
    }

    void set_state(ChunkState state) {
        _state.store(state);
    }

    ChunkState state() const {
        return _state.load();
    }

    glm::vec2 position() const {
        return glm::vec2(_x, _y);
    }

    static uint64_t id(int _x, int _y) {
#define _INDEX(I) (abs((I) * 2) - ((I) > 0 ? 1 : 0))
        int x = _INDEX(_x), y = _INDEX(_y);
        return x >= y ? x * x + x + y : x + y * y;
#undef INDEX
    }

    uint64_t id() const {
        return Chunk::id(_x, _y);
    }

    static Rect bounds(int _x, int _y) {
        return {
            .x = _x * CHUNK_WIDTH * TILE_WIDTH,
            .y = _y * CHUNK_HEIGHT * TILE_HEIGHT,
            .w = CHUNK_WIDTH * TILE_WIDTH,
            .h = CHUNK_HEIGHT * TILE_HEIGHT
        };
    }

    Rect bounds() const {
        return Chunk::bounds(_x, _y);
    }

    void fill() {
        std::lock_guard<std::mutex> lock(_chunk_mutex);

        uint8_t _grid[CHUNK_SIZE];
        cellular_automata(CHUNK_WIDTH, CHUNK_HEIGHT,
                          CHUNK_FILL_CHANCE, CHUNK_SMOOTH_ITERATIONS,
                          CHUNK_SURVIVE, CHUNK_STARVE,
                          _grid);
        for (int y = 0; y < CHUNK_HEIGHT; y++)
            for (int x = 0; x < CHUNK_WIDTH; x++)
                _tiles[x][y].solid = x == 0 || y == 0 || x == CHUNK_WIDTH - 1 || y == CHUNK_HEIGHT - 1 ? 1 : _grid[y * CHUNK_WIDTH + x];
        for (int y = 0; y < CHUNK_HEIGHT; y++)
            for (int x = 0; x < CHUNK_WIDTH; x++)
                _tiles[x][y].bitmask = _tiles[x][y].solid ? tile_bitmask(this, x, y, 1) : 0;
        dirty.store(true);
    }

    bool draw(glm::mat4 *mvp = nullptr) {
        if (_state.load() != ChunkState::CHUNK_STATE_ACTIVE)
            return false;

        if (dirty.load())
            build();
        if (mvp != nullptr) {
            std::lock_guard<std::mutex> lock(_chunk_mutex);
            _mvp = glm::translate(*mvp, glm::vec3(_x * CHUNK_WIDTH * TILE_WIDTH, _y * CHUNK_HEIGHT * TILE_HEIGHT, 0.f));
        }
        if (sg_query_buffer_state(_bind.vertex_buffers[0]) != SG_RESOURCESTATE_VALID)
            return false;

        sg_apply_bindings(_bind);
        vs_params_t vs_params = { .mvp = _mvp };
        sg_range params = SG_RANGE(vs_params);
        sg_apply_uniforms(UB_vs_params, &params);
        sg_draw(0, CHUNK_SIZE * 6, 1);
        return true;
    }

    void each(const std::function<void(int, int, Tile&)>& func) {
        std::lock_guard<std::mutex> lock(_chunk_mutex);
        for (int x = 0; x < CHUNK_WIDTH; x++)
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                Tile &tile = _tiles[x][y];
                func(x, y, tile);
            }
    };
};
