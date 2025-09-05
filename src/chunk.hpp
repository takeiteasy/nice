//
//  chunks.hpp
//  nice
//
//  Created by George Watson on 14/08/2025.
//

#pragma once

#include "nice.hpp"
#include "vertex_batch.hpp"
#include "camera.hpp"
#include <unordered_map>
#include <shared_mutex>
#include <random>
#include <iostream>
#include <fstream>
#include "fmt/format.h"
#include "basic.glsl.h"

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
        uint8_t visited;
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

struct ChunkVertex {
    glm::vec2 position;
    glm::vec2 texcoord;
};

class Chunk {
    int _x, _y;
    Tile _tiles[CHUNK_WIDTH][CHUNK_HEIGHT];
    mutable std::shared_mutex _chunk_mutex;
    VertexBatch<ChunkVertex, CHUNK_SIZE * 6, false> _batch;
    std::atomic<bool> _is_filled = false;
    std::atomic<bool> _is_built = false;
    std::atomic<bool> _is_destroyed = false;
    std::atomic<ChunkVisibility> _visibility = ChunkVisibility::OutOfSign;
    glm::mat4 _mvp;
    Camera *_camera;
    Texture *_texture;
    std::atomic<bool> _rebuild_mvp = true;

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

    ChunkVertex *generate_quad(glm::vec2 position, glm::vec2 clip_offset) {
        int _x = static_cast<int>(position.x);
        int _y = static_cast<int>(position.y);
        glm::vec2 _positions[] = {
            {_x, _y},                            // Top-left
            {_x + TILE_WIDTH, _y},               // Top-right
            {_x + TILE_WIDTH, _y + TILE_HEIGHT}, // Bottom-right
            {_x, _y + TILE_HEIGHT},              // Bottom-left
        };

        int _clip_x = static_cast<int>(clip_offset.x);
        int _clip_y = static_cast<int>(clip_offset.y);
        int _texture_width = _texture->width();
        int _texture_height = _texture->height();
        float iw = 1.f / _texture_width;
        float ih = 1.f / _texture_height;
        float tl = _clip_x * iw;
        float tt = _clip_y * ih;
        float tr = (_clip_x + TILE_ORIGINAL_WIDTH) * iw;
        float tb = (_clip_y + TILE_ORIGINAL_HEIGHT) * ih;
        glm::vec2 _texcoords[4] = {
            {tl, tt}, // top left
            {tr, tt}, // top right
            {tr, tb}, // bottom right
            {tl, tb}, // bottom left
        };

        ChunkVertex *v = new ChunkVertex[6];
        static uint16_t _indices[] = {0, 1, 2, 2, 3, 0};
        for (int i = 0; i < 6; i++) {
            v[i].position = _positions[_indices[i]];
            v[i].texcoord = _texcoords[_indices[i]];
        }
        return v;
    }

    struct ChunkHeader {
        uint32_t magic;        // "NICE"
        uint32_t version;      // Version number
        uint32_t width;        // Width in tiles
        uint32_t height;       // Height in tiles
        uint32_t flags;        // Optimization flags
    };

    struct ChunkRLEEntry {
        uint32_t count;    // How many consecutive tiles
        uint8_t value;     // The field value
    };

    struct ChunkSparseEntry {
        uint32_t index;    // Tile index
        uint8_t value;     // The field value
    };

    enum ChunkOptimizationFlags {
        SOLID_RLE = 1 << 0,     // Solid field uses RLE
        VISITED_RLE = 1 << 1,   // Visited field uses RLE  
        EXTRA_RLE = 1 << 2,     // Extra field uses RLE
        SOLID_SPARSE = 1 << 3,  // Solid field uses sparse
        VISITED_SPARSE = 1 << 4,// Visited field uses sparse
        EXTRA_SPARSE = 1 << 5   // Extra field uses sparse
    };

    // Calculate compression efficiency for a field
    template<typename FieldGetter>
    std::pair<size_t, size_t> _calculate_compression_efficiency(FieldGetter getter) const {
        // Calculate RLE size
        uint8_t current_value = getter(_tiles[0][0]);
        size_t rle_runs = 1;
        uint32_t current_count = 1;
        for (int y = 0; y < CHUNK_HEIGHT; y++)
            for (int x = 0; x < CHUNK_WIDTH; x++) {
                if (x == 0 && y == 0)
                    continue;
                uint8_t value = getter(_tiles[x][y]);
                if (value == current_value && current_count < UINT32_MAX)
                    current_count++;
                else {
                    rle_runs++;
                    current_value = value;
                    current_count = 1;
                }
            }
        
        // Calculate sparse size (only non-zero values)
        size_t sparse_count = 0;
        for (int y = 0; y < CHUNK_HEIGHT; y++)
            for (int x = 0; x < CHUNK_WIDTH; x++)
                if (getter(_tiles[x][y]) != 0)
                    sparse_count++;
        size_t rle_size = sizeof(uint32_t) + rle_runs * sizeof(ChunkRLEEntry);
        size_t sparse_size = sizeof(uint32_t) + sparse_count * sizeof(ChunkSparseEntry);
        return {rle_size, sparse_size};
    }

    template<typename FieldGetter>
    void _serialize_field_rle(std::ofstream& file, FieldGetter getter) const {
        std::vector<ChunkRLEEntry> encoded;
        uint8_t current_value = getter(_tiles[0][0]);
        uint32_t current_count = 1;
        for (int y = 0; y < CHUNK_HEIGHT; y++)
            for (int x = 0; x < CHUNK_WIDTH; x++) {
                if (x == 0 && y == 0)
                    continue;
                uint8_t value = getter(_tiles[x][y]);
                if (value == current_value && current_count < UINT32_MAX)
                    current_count++;
                else {
                    encoded.push_back({current_count, current_value});
                    current_value = value;
                    current_count = 1;
                }
            }
        encoded.push_back({current_count, current_value});

        uint32_t entryCount = static_cast<uint32_t>(encoded.size());
        file.write(reinterpret_cast<const char*>(&entryCount), sizeof(entryCount));
        for (const auto& entry : encoded)
            file.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
    }

    template<typename FieldGetter>
    void _serialize_field_sparse(std::ofstream& file, FieldGetter getter) const {
        std::vector<ChunkSparseEntry> sparse_data;
        for (int y = 0; y < CHUNK_HEIGHT; y++)
            for (int x = 0; x < CHUNK_WIDTH; x++) {
                uint8_t value = getter(_tiles[x][y]);
                if (value != 0) {
                    uint32_t index = y * CHUNK_WIDTH + x;
                    sparse_data.push_back({index, value});
                }
            }

        uint32_t entryCount = static_cast<uint32_t>(sparse_data.size());
        file.write(reinterpret_cast<const char*>(&entryCount), sizeof(entryCount));
        for (const auto& entry : sparse_data)
            file.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
    }

    // Generic field serializer that chooses compression method based on efficiency
    template<typename FieldGetter>
    uint32_t _serialize_field(std::ofstream& file, FieldGetter getter, uint32_t rle_flag, uint32_t sparse_flag) const {
        auto [rle_size, sparse_size] = _calculate_compression_efficiency(getter);
        if (rle_size <= sparse_size) {
            _serialize_field_rle(file, getter);
            return rle_flag;
        } else {
            _serialize_field_sparse(file, getter);
            return sparse_flag;
        }
    }

    template<typename FieldSetter>
    void _deserialize_field_rle(std::ifstream& file, FieldSetter setter) {
        uint32_t count;
        file.read(reinterpret_cast<char*>(&count), sizeof(count));
        
        int tileIdx = 0;
        for (uint32_t entryIdx = 0; entryIdx < count; ++entryIdx) {
            ChunkRLEEntry entry;
            file.read(reinterpret_cast<char*>(&entry), sizeof(entry));
            for (uint32_t i = 0; i < entry.count && tileIdx < CHUNK_SIZE; ++i, ++tileIdx) {
                int x = tileIdx % CHUNK_WIDTH;
                int y = tileIdx / CHUNK_WIDTH;
                setter(_tiles[x][y], entry.value);
            }
        }
    }

    template<typename FieldSetter>
    void _deserialize_field_sparse(std::ifstream& file, FieldSetter setter) {
        uint32_t count;
        file.read(reinterpret_cast<char*>(&count), sizeof(count));
        for (uint32_t i = 0; i < count; ++i) {
            ChunkSparseEntry entry;
            file.read(reinterpret_cast<char*>(&entry), sizeof(entry));
            if (entry.index < CHUNK_SIZE) {
                int x = entry.index % CHUNK_WIDTH;
                int y = entry.index / CHUNK_WIDTH;
                setter(_tiles[x][y], entry.value);
            }
        }
    }

    // Generic field deserializer that chooses method based on flags
    template<typename FieldSetter>
    void _deserialize_field(std::ifstream& file, FieldSetter setter, uint32_t flags, uint32_t rle_flag) {
        if (flags & rle_flag)
            _deserialize_field_rle(file, setter);
        else
            _deserialize_field_sparse(file, setter);
    }

    void _serialize(std::ofstream& file) const {
        uint32_t flags = 0;
        auto flags_pos = file.tellp();
        file.write(reinterpret_cast<const char*>(&flags), sizeof(flags));
        
        // Serialize each field and accumulate flags
        flags |= _serialize_field(file, [](const Tile& t) { return t.solid; }, SOLID_RLE, SOLID_SPARSE);
        flags |= _serialize_field(file, [](const Tile& t) { return t.visited; }, VISITED_RLE, VISITED_SPARSE);
        flags |= _serialize_field(file, [](const Tile& t) { return t.extra; }, EXTRA_RLE, EXTRA_SPARSE);
        
        // Go back and write the actual flags
        auto end_pos = file.tellp();
        file.seekp(flags_pos);
        file.write(reinterpret_cast<const char*>(&flags), sizeof(flags));
        file.seekp(end_pos);
    }
    
    void _deserialize(std::ifstream& file, uint32_t flags) {
        // Deserialize each field based on flags
        _deserialize_field(file, [](Tile& t, uint8_t v) { t.solid = v; }, flags, SOLID_RLE);
        _deserialize_field(file, [](Tile& t, uint8_t v) { t.visited = v; }, flags, VISITED_RLE);
        _deserialize_field(file, [](Tile& t, uint8_t v) { t.extra = v; }, flags, EXTRA_RLE);
    }

public:
    Chunk(int x, int y, Camera *camera, Texture *texture)
        : _camera(camera)
        , _texture(texture)
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

    std::pair<ChunkVertex*, size_t> vertices() {
        std::shared_lock<std::shared_mutex> lock(_chunk_mutex);
        
        // First, count solid tiles to allocate the correct amount of memory
        size_t solid_count = 0;
        for (int x = 0; x < CHUNK_WIDTH; x++)
            for (int y = 0; y < CHUNK_HEIGHT; y++)
                if (_tiles[x][y].solid)
                    solid_count++;

        ChunkVertex *vertices = new ChunkVertex[solid_count * 6];
        size_t vertex_index = 0;
        for (int x = 0; x < CHUNK_WIDTH; x++)
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                Tile *tile = &_tiles[x][y];
                if (!tile->solid)
                    continue;
                glm::vec2 clip = Autotile3x3Simplified[tile->bitmask];
                int clip_x = static_cast<int>((clip.x * TILE_ORIGINAL_WIDTH) + ((clip.x + 1) * TILE_PADDING));
                int clip_y = static_cast<int>((clip.y * TILE_ORIGINAL_HEIGHT) + ((clip.y + 1) * TILE_PADDING));
                ChunkVertex *veritces = generate_quad({x * TILE_WIDTH, y * TILE_HEIGHT},
                                                      {clip_x, clip_y});
                std::copy(veritces, veritces + 6, &vertices[vertex_index]);
                delete[] veritces;
                vertex_index += 6;
            }
        return {vertices, solid_count * 6};
    }

    bool build() {
        if (!is_filled())
            return false;
        
        // Get vertices while holding the lock
        auto [_vertices, vertex_count] = vertices();
        
        // Now acquire unique lock for modification
        std::unique_lock<std::shared_mutex> lock(_chunk_mutex);
        _batch.add_vertices(_vertices, vertex_count);
        _batch.build();
        lock.unlock();
        
        delete[] _vertices;
        _is_built.store(true);
        return true;
    }

    std::vector<glm::vec2> poisson(float r, int k=30, bool invert=false, bool lock = true, int max_tries=CHUNK_SIZE / 4, Rect region={0, 0, CHUNK_WIDTH, CHUNK_HEIGHT}) {
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
        int region_width = std::min(region.w, CHUNK_WIDTH - region.x);
        int region_height = std::min(region.h, CHUNK_HEIGHT - region.y);

        // Ensure the region is valid
        if (region_width <= 0 || region_height <= 0 ||  region.x >= CHUNK_WIDTH || region.x >= CHUNK_HEIGHT)
            return {};

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> random_dis(0.0f, 1.0f);
        std::optional<std::shared_lock<std::shared_mutex>> _lock;
        if (lock)
            _lock.emplace(_chunk_mutex);
        int tries = 0;
        glm::vec2 p;
        while (tries++ < max_tries) {
            float px = region.x + region_width * random_dis(gen);
            float py = region.y + region_height * random_dis(gen);
            p = glm::vec2(px, py);

            int tile_x = static_cast<int>(p.x);
            int tile_y = static_cast<int>(p.y);
            if (tile_x >= 0 && tile_x < CHUNK_WIDTH && tile_y >= 0 && tile_y < CHUNK_HEIGHT &&
                static_cast<bool>(_tiles[tile_x][tile_y].solid) == invert)
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

            for (int i = 0; i < k; i++) {
                float alpha = 2.0f * M_PI * random_dis(gen);
                float d = r * std::sqrt(3.0f * random_dis(gen) + 1.0f);
                float px = point.x + d * std::cos(alpha);
                float py = point.y + d * std::sin(alpha);

                // Check if point is within the specified region bounds
                if (!(region.x <= px && px < region.x + region_width &&
                      region.y <= py && py < region.y + region_height))
                    continue;

                // Check if point is within chunk bounds
                if (!(0 <= px && px < CHUNK_WIDTH && 0 <= py && py < CHUNK_HEIGHT))
                    continue;

                // Check if tile is solid
                int tile_x = static_cast<int>(px);
                int tile_y = static_cast<int>(py);
                if (static_cast<bool>(_tiles[tile_x][tile_y].solid) != invert)
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
        if (lock && _lock.has_value())
            _lock.value().unlock();

        std::vector<glm::vec2> points;
        for (int x = 0; x < grid_width; x++)
            for (int y = 0; y < grid_height; y++)
                if (grid[x][y] != nullptr) {
                    // Only include points that are within the specified region
                    glm::vec2 point = *grid[x][y];
                    if (point.x >= region.x && point.x < region.x + region_width &&
                        point.y >= region.y && point.y < region.y + region_height)
                        points.push_back(point);
                    delete grid[x][y];
                }
        return points;
    }

    struct AStarNode {
        glm::ivec2 position;
        AStarNode *parent;
        float g;                          // Cost from start to current node
        float h;                          // Heuristic cost from current node to end
        float f() const { return g + h; } // Total cost
    };

    std::optional<std::vector<glm::vec2>> astar(glm::vec2 start, glm::vec2 end, int max_steps=3, bool lock=true) {
        if (!is_filled())
            return std::nullopt;
        if (start.x < 0 || start.x >= CHUNK_WIDTH || start.y < 0 || start.y >= CHUNK_HEIGHT)
            return std::nullopt;
        if (end.x < 0 || end.x >= CHUNK_WIDTH || end.y < 0 || end.y >= CHUNK_HEIGHT)
            return std::nullopt;
        int start_x = static_cast<int>(start.x);
        int start_y = static_cast<int>(start.y);
        int end_x = static_cast<int>(end.x);
        int end_y = static_cast<int>(end.y);
        if (_tiles[start_x][start_y].solid || _tiles[end_x][end_y].solid)
            return std::nullopt;
        std::optional<std::shared_lock<std::shared_mutex>> _lock;
        if (lock)
            _lock.emplace(_chunk_mutex);
        auto heuristic = [](glm::ivec2 a, glm::ivec2 b) {
            return glm::length(glm::vec2(b - a));
        };
        std::vector<AStarNode*> open_list;
        std::vector<glm::ivec2> closed_list;
        AStarNode *start_node = new AStarNode{glm::ivec2(start_x, start_y), nullptr, 0.0f, heuristic(glm::ivec2(start_x, start_y), glm::ivec2(end_x, end_y))};
        open_list.push_back(start_node);
        AStarNode *end_node = nullptr;
        int steps = 0;
        while (!open_list.empty() && steps < max_steps) {
            steps++;
            // Find node with lowest f in open list
            auto current_it = std::min_element(open_list.begin(), open_list.end(), [](AStarNode* a, AStarNode* b) {
                return a->f() < b->f();
            });
            AStarNode *current_node = *current_it;
            // If we reached the end, reconstruct path
            if (current_node->position == glm::ivec2(end_x, end_y)) {
                end_node = current_node;
                break;
            }
            open_list.erase(current_it);
            closed_list.push_back(current_node->position);
            // Explore neighbors (4-directional)
            std::vector<glm::ivec2> neighbors = {
                current_node->position + glm::ivec2(0, -1),
                current_node->position + glm::ivec2(1, 0),
                current_node->position + glm::ivec2(0, 1),
                current_node->position + glm::ivec2(-1, 0)
            };
            for (const auto& neighbor_pos : neighbors) {
                if (neighbor_pos.x < 0 || neighbor_pos.x >= CHUNK_WIDTH || neighbor_pos.y < 0 || neighbor_pos.y >= CHUNK_HEIGHT)
                    continue;
                if (_tiles[neighbor_pos.x][neighbor_pos.y].solid)
                    continue;
                if (std::find(closed_list.begin(), closed_list.end(), neighbor_pos) != closed_list.end())
                    continue;
                float tentative_g = current_node->g + 1.0f; // Assume cost between nodes is 1
                auto open_it = std::find_if(open_list.begin(), open_list.end(), [&neighbor_pos](AStarNode* node) {
                    return node->position == neighbor_pos;
                });
                if (open_it == open_list.end()) {
                    AStarNode *neighbor_node = new AStarNode{neighbor_pos, current_node, tentative_g, heuristic(neighbor_pos, glm::ivec2(end_x, end_y))};
                    open_list.push_back(neighbor_node);
                } else if (tentative_g < (*open_it)->g) {
                    (*open_it)->g = tentative_g;
                    (*open_it)->parent = current_node;
                }
            }
        }
        if (lock && _lock.has_value())
            _lock.value().unlock();
        std::vector<glm::vec2> path;
        if (end_node != nullptr) {
            AStarNode *node = end_node;
            while (node != nullptr) {
                path.push_back(glm::vec2(node->position));
                node = node->parent;
            }
            std::reverse(path.begin(), path.end());
        }
        // Clean up allocated nodes
        for (AStarNode* node : open_list)
            delete node;
        if (end_node != nullptr)
            delete end_node;
        return path.empty() ? std::nullopt : std::optional<std::vector<glm::vec2>>(path);
    }

    void draw(bool force_update = false) {
        if (!is_ready())
            return;

        if (_rebuild_mvp.load() || force_update) {
            _mvp = glm::translate(_camera->matrix(),
                                  glm::vec3(_x * CHUNK_WIDTH * TILE_WIDTH,
                                            _y * CHUNK_HEIGHT * TILE_HEIGHT,
                                            0.f));
            _rebuild_mvp.store(false);
        }

        std::shared_lock<std::shared_mutex> lock(_chunk_mutex);
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
        return Rect(_x * CHUNK_WIDTH * TILE_WIDTH,
                    _y * CHUNK_HEIGHT * TILE_HEIGHT,
                    CHUNK_WIDTH * TILE_WIDTH,
                    CHUNK_HEIGHT * TILE_HEIGHT);
    }

    uint64_t id() const {
        return index(_x, _y);
    }

    Rect bounds() const {
        return Chunk::bounds(_x, _y);
    }

    int x() const {
        return _x;
    }

    int y() const {
        return _y;
    }
    
    bool is_filled() const {
        return _is_filled.load();
    }

    bool is_built() const {
        return _is_built.load();
    }

    bool is_ready() const {
        return is_filled() && is_built() && !is_destroyed();
    }

    bool is_destroyed() const {
        return _is_destroyed.load();
    }

    void mark_destroyed() {
        _is_destroyed.store(true);
    }

    ChunkVisibility visibility() const {
        return _visibility.load();
    }
    
    void set_visibility(ChunkVisibility visibility) {
        _visibility.store(visibility);
    }

    bool is_walkable(int tx, int ty, bool lock=true) const {
        if (tx < 0 || tx >= CHUNK_WIDTH || ty < 0 || ty >= CHUNK_HEIGHT)
            return false;
        if (lock)
            std::shared_lock<std::shared_mutex> lock(_chunk_mutex);
        return !_tiles[tx][ty].solid;
    }

    bool serialize(const char *path) const {
        if (!_is_filled.load())
            return false;

        // Acquire shared lock to prevent modifications during serialization
        std::shared_lock<std::shared_mutex> lock(_chunk_mutex);

        std::ofstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error(fmt::format("Failed to open file for writing: {}", path));

        ChunkHeader header = {
            .magic = 0x4543494E, // "NICE"
            .version = 1,        // Optimized RLE format
            .width = CHUNK_WIDTH,
            .height = CHUNK_HEIGHT,
            .flags = 0           // Will be written by _serialize
        };
        file.write(reinterpret_cast<const char*>(&header), sizeof(ChunkHeader));

        try {
            _serialize(file);
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string("Failed to serialize chunk: ") + e.what());
        }
        return true;
    }

    void deserialize(const char *path) {
        // Acquire unique lock to prevent other operations during deserialization
        std::unique_lock<std::shared_mutex> lock(_chunk_mutex);

        std::ifstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error(fmt::format("Failed to open file for reading: {}", path));

        ChunkHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(ChunkHeader));
        if (header.magic != 0x4543494E) // "NICE"
            throw std::runtime_error("Invalid chunk data (bad magic)");
        if (header.version != 1)
            throw std::runtime_error("Unsupported chunk version");
        if (header.width != CHUNK_WIDTH || header.height != CHUNK_HEIGHT)
            throw std::runtime_error("Chunk size mismatch");

        try {
            memset(_tiles, 0, sizeof(_tiles));
            
            // Optimized format with field-specific compression
            uint32_t flags;
            file.read(reinterpret_cast<char*>(&flags), sizeof(flags));
            _deserialize(file, flags);
            
            // Recalculate bitmasks after deserialization
            for (int y = 0; y < CHUNK_HEIGHT; y++)
                for (int x = 0; x < CHUNK_WIDTH; x++)
                    _tiles[x][y].bitmask = _tiles[x][y].solid ? tile_bitmask(this, x, y, 1) : 0;
            
            _is_filled.store(true);
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string("Failed to deserialize chunk: ") + e.what());
        }
    }
};
