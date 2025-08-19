//
//  chunks.hpp
//  rpg
//
//  Created by George Watson on 14/08/2025.
//

#pragma once

#include "global.hpp"
#include "vertex_batch.hpp"
#include "camera.hpp"
#include "entity.hpp"
#include <unordered_map>
#include <shared_mutex>
#include <random>

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

union Tile {
    struct {
        uint8_t bitmask;
        uint8_t visible;
        uint8_t solid;
        uint8_t extra;
    };
    uint32_t value;
};

enum class ChunkVisibility {
    OutOfSign,
    Visible,
    Occluded
};

class Chunk: Entity<> {
    int _x, _y;
    Tile _tiles[CHUNK_WIDTH][CHUNK_HEIGHT];
    mutable std::shared_mutex _chunk_mutex;
    VertexBatch<BasicVertex, CHUNK_SIZE * 6, false> _batch;
    std::atomic<bool> _is_filled = false;
    std::atomic<bool> _is_built = false;
    std::atomic<bool> _is_destroyed = false;
    std::atomic<ChunkVisibility> _visibility = ChunkVisibility::OutOfSign;
    glm::mat4 _mvp;
    Camera *_camera;
    bool _rebuild_mvp = true;

    static void cellular_automata(int width, int height, int fill_chance, int smooth_iterations, int survive, int starve, uint8_t* result) {
        memset(result, 0, width * height * sizeof(uint8_t));
        // Randomly fill the grid
        std::random_device seed;
        std::mt19937 gen{seed()};
        std::uniform_int_distribution<> ud{1, 100};
        for (int x = 0; x < width; x++)
            for (int y = 0; y < height; y++) {
                int min_dist_from_edge = std::min(std::min(x, width - 1 - x), std::min(y, height - 1 - y));
                int edge_bonus = std::max(0, 30 - (min_dist_from_edge * 5)); // 30% bonus at edges, decreasing toward center
                int adjusted_fill_chance = fill_chance + edge_bonus;
                if (adjusted_fill_chance > 100) adjusted_fill_chance = 100;
                result[y * width + x] = ud(gen) <= adjusted_fill_chance;
            }
        // Run cellular-automata on grid n times
        for (int i = 0; i < std::max(smooth_iterations, 1); i++)
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
    Chunk(int x, int y, Camera *camera, Texture *texture)
        : Entity<>({x * TILE_WIDTH, y * TILE_HEIGHT}, x, y, texture->width(), texture->height())
        , _camera(camera)
        , _x(x)
        , _y(y) {
        _batch.set_texture(texture);
    };

    bool fill() {
        if (is_filled())
            return false;

        std::unique_lock<std::shared_mutex> lock(_chunk_mutex);

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

        _is_filled.store(true);
        return true;
    }

    std::pair<BasicVertex*, size_t> vertices() override {
        // First, count solid tiles to allocate the correct amount of memory
        size_t solid_count = 0;
        for (int x = 0; x < CHUNK_WIDTH; x++)
            for (int y = 0; y < CHUNK_HEIGHT; y++)
                if (_tiles[x][y].solid)
                    solid_count++;

        BasicVertex *vertices = new BasicVertex[solid_count * 6];
        size_t vertex_index = 0;
        for (int x = 0; x < CHUNK_WIDTH; x++)
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                Tile *tile = &_tiles[x][y];
                if (!tile->solid)
                    continue;
                glm::vec2 clip = Autotile3x3Simplified[tile->bitmask];
                int clip_x = static_cast<int>((clip.x * TILE_ORIGINAL_WIDTH) + ((clip.x + 1) * TILE_PADDING));
                int clip_y = static_cast<int>((clip.y * TILE_ORIGINAL_HEIGHT) + ((clip.y + 1) * TILE_PADDING));
                BasicVertex *veritces = generate_quad({x * TILE_WIDTH, y * TILE_HEIGHT},
                                                      {TILE_WIDTH, TILE_HEIGHT},
                                                      {clip_x, clip_y},
                                                      {TILE_ORIGINAL_WIDTH, TILE_ORIGINAL_HEIGHT},
                                                      {_texture_width, _texture_height});
                std::copy(veritces, veritces + 6, &vertices[vertex_index]);
                delete[] veritces;
                vertex_index += 6;
            }
        return {vertices, solid_count * 6};
    }

    bool build() {
        if (!is_filled())
            return false;
        std::unique_lock<std::shared_mutex> lock(_chunk_mutex);
        auto [_vertices, vertex_count] = vertices();
        _batch.add_vertices(_vertices, vertex_count);
        lock.unlock();
        _batch.build();
        delete[] _vertices;
        _is_built.store(true);
        return true;
    }

    // TODO: Replace manual solid check with custom function callback
    template<int K=30, bool Invert=false, bool Lock=true>
    std::vector<glm::vec2> poisson(float r, int offset_x=0, int offset_y=0, int max_width=CHUNK_WIDTH, int max_height=CHUNK_HEIGHT, int max_tries=CHUNK_SIZE / 4) {
        float cell_size = r / std::sqrt(2.0f);
        int grid_width = static_cast<int>(std::ceil(CHUNK_WIDTH / cell_size));
        int grid_height = static_cast<int>(std::ceil(CHUNK_HEIGHT / cell_size));
        std::vector<std::vector<glm::vec2*>> grid(grid_width, std::vector<glm::vec2*>(grid_height, nullptr));

        auto grid_coords = [cell_size](glm::vec2 point) {
            return glm::vec2(static_cast<int>(std::floor(point.x / cell_size)),
                             static_cast<int>(std::floor(point.y / cell_size)));
        };

        auto fits = [&grid, grid_width, grid_height, r](glm::vec2 p, int gx, int gy) {
            for (int x = std::max(gx - 2, 0); x < std::min(gx + 3, grid_width); x++)
                for (int y = std::max(gy - 2, 0); y < std::min(gy + 3, grid_height); y++) {
                    glm::vec2* g = grid[x][y];
                    if (g == nullptr)
                        continue;
                    if (glm::distance(p, *g) <= r)
                        return false;
                }
            return true;
        };

        // Calculate the actual region bounds
        int region_width = std::min(max_width, CHUNK_WIDTH - offset_x);
        int region_height = std::min(max_height, CHUNK_HEIGHT - offset_y);

        // Ensure the region is valid
        if (region_width <= 0 || region_height <= 0 ||  offset_x >= CHUNK_WIDTH || offset_y >= CHUNK_HEIGHT)
            return {};

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> random_dis(0.0f, 1.0f);
        std::optional<std::shared_lock<std::shared_mutex>> lock;
        if (Lock)
            lock.emplace(_chunk_mutex);
        int tries = 0;
        glm::vec2 p;
        while (tries++ < max_tries) {
            float px = offset_x + region_width * random_dis(gen);
            float py = offset_y + region_height * random_dis(gen);
            p = glm::vec2(px, py);

            int tile_x = static_cast<int>(p.x);
            int tile_y = static_cast<int>(p.y);
            if (tile_x >= 0 && tile_x < CHUNK_WIDTH && tile_y >= 0 && tile_y < CHUNK_HEIGHT &&
                static_cast<bool>(_tiles[tile_x][tile_y].solid) == Invert)
                break;
        }
        if (tries >= max_tries)
            return {};
        glm::vec2 gp = grid_coords(p);
        grid[static_cast<int>(gp.x)][static_cast<int>(gp.y)] = new glm::vec2(p);
        std::vector<glm::vec2> queue = {p};

        while (!queue.empty()) {
            size_t qi = static_cast<size_t>(random_dis(gen) * queue.size());
            if (qi >= queue.size())
                qi = queue.size() - 1;
            glm::vec2 point = queue[qi];
            queue[qi] = queue.back();
            queue.pop_back();

            for (int i = 0; i < K; i++) {
                float alpha = 2.0f * M_PI * random_dis(gen);
                float d = r * std::sqrt(3.0f * random_dis(gen) + 1.0f);
                float px = point.x + d * std::cos(alpha);
                float py = point.y + d * std::sin(alpha);

                // Check if point is within the specified region bounds
                if (!(offset_x <= px && px < offset_x + region_width &&
                      offset_y <= py && py < offset_y + region_height))
                    continue;

                // Check if point is within chunk bounds
                if (!(0 <= px && px < CHUNK_WIDTH && 0 <= py && py < CHUNK_HEIGHT))
                    continue;

                // Check if tile is solid
                int tile_x = static_cast<int>(px);
                int tile_y = static_cast<int>(py);
                if (static_cast<bool>(_tiles[tile_x][tile_y].solid) != Invert)
                    continue;

                glm::vec2 new_point = glm::vec2(px, py);
                glm::vec2 grid_pos = grid_coords(new_point);
                int gx = static_cast<int>(grid_pos.x);
                int gy = static_cast<int>(grid_pos.y);
                if (!fits(new_point, gx, gy))
                    continue;
                queue.push_back(new_point);
                grid[gx][gy] = new glm::vec2(new_point);
            }
        }
        if (Lock && lock.has_value())
            lock.value().unlock();

        std::vector<glm::vec2> points;
        for (int x = 0; x < grid_width; x++)
            for (int y = 0; y < grid_height; y++)
                if (grid[x][y] != nullptr) {
                    // Only include points that are within the specified region
                    glm::vec2 point = *grid[x][y];
                    if (point.x >= offset_x && point.x < offset_x + region_width &&
                        point.y >= offset_y && point.y < offset_y + region_height)
                        points.push_back(point);
                    delete grid[x][y];
                }
        return points;
    }

    void draw(bool force_update = false) {
        if (!is_ready())
            return;

        if (_rebuild_mvp || force_update) {
            _mvp = glm::translate(_camera->matrix(),
                                  glm::vec3(_x * CHUNK_WIDTH * TILE_WIDTH,
                                            _y * CHUNK_HEIGHT * TILE_HEIGHT,
                                            0.f));
            _rebuild_mvp = false;
        }

        std::lock_guard<std::shared_mutex> lock(_chunk_mutex);
        vs_params_t vs_params = { .mvp = _mvp };
        sg_range params = SG_RANGE(vs_params);
        sg_apply_uniforms(UB_vs_params, &params);
        _batch.flush();
    }

    static inline std::string visibility_to_string(ChunkVisibility visibility) {
        switch (visibility) {
            case ChunkVisibility::OutOfSign:
                return "None";
            case ChunkVisibility::Visible:
                return "Visible";
            case ChunkVisibility::Occluded:
                return "Occluded";
        }
    }

    static Rect bounds(int _x, int _y) {
        return {
            .x = _x * CHUNK_WIDTH * TILE_WIDTH,
            .y = _y * CHUNK_HEIGHT * TILE_HEIGHT,
            .w = CHUNK_WIDTH * TILE_WIDTH,
            .h = CHUNK_HEIGHT * TILE_HEIGHT
        };
    }

    uint64_t id() const { return index(_x, _y); }
    Rect bounds() const { return Chunk::bounds(_x, _y); }
    int x() const { return _x; }
    int y() const { return _y; }
    bool is_filled() const { return _is_filled.load(); }
    bool is_built() const { return _is_built.load(); }
    bool is_ready() const { return is_filled() && is_built() && !is_destroyed(); }
    bool is_destroyed() const { return _is_destroyed.load(); }
    void mark_destroyed() { _is_destroyed.store(true); }
    ChunkVisibility visibility() const { return _visibility.load(); }
    void set_visibility(ChunkVisibility visibility) { _visibility.store(visibility); }
};
