//
//  chunks.hh
//  rpg
//
//  Created by George Watson on 14/08/2025.
//

#pragma once

#include "asset_manager.hh"
#include "batch.hh"
#include "fmt/format.h"
#include "camera.hh"
#include "jobs.hh"
#include <unordered_map>
#include <iostream>
#include "ores.hh"
#include <shared_mutex>
#include <random>
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

struct ChunkVertex {
    glm::vec2 position;
    glm::vec2 texcoord;
};

typedef VertexBatch<ChunkVertex, CHUNK_SIZE * 6, false> ChunkVertexBatch;

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

class Chunk {
    int _x, _y, _texture_width, _texture_height;
    Tile _tiles[CHUNK_WIDTH][CHUNK_HEIGHT];
    mutable std::shared_mutex _chunk_mutex;
    ChunkVertexBatch _batch;
    std::atomic<bool> _is_filled = false;
    std::atomic<bool> _is_built = false;
    std::atomic<bool> _is_destroyed = false;
    std::atomic<ChunkVisibility> _visibility = ChunkVisibility::OutOfSign;
    glm::mat4 _mvp;
    Camera *_camera;
    bool _rebuild_mvp = true;
    OreManager *_ore_manager;

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
    Chunk(int x, int y, Camera *camera, Texture *texture): _x(x), _y(y), _camera(camera), _texture_width(texture->width()), _texture_height(texture->height()) {
        auto ore_texture = $Assets.get<Texture>("assets/ores.exploded.png");
        if (!ore_texture.has_value())
            throw std::runtime_error(fmt::format("Failed to load ore texture for chunk at ({}, {})", x, y));
        _ore_manager = new OreManager(camera, ore_texture.value(), x, y);
        _batch.set_texture(texture);
    };

    ~Chunk() {
        delete _ore_manager;
    }

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

        // TODO: Run this in a separate thread
        std::random_device seed;
        std::mt19937 gen{seed()};
        std::uniform_int_distribution<> ro{1, static_cast<int>(OreType::COUNT) - 1};
        // TODO: Check if tile is an edge and reject, ore overlaps wall sprite
        std::vector<glm::vec2> ore_positions = poisson<30, true, false>(5);
        std::vector<OreType> ore_types;
        ore_types.reserve(ore_positions.size());
        for (int i = 0; i < ore_positions.size(); i++)
            ore_types.push_back(static_cast<OreType>(ro(gen)));

        std::vector<std::pair<OreType, glm::vec2>> ores;
        ores.reserve(ore_positions.size());
        for (size_t i = 0; i < ore_positions.size(); ++i)
            ores.emplace_back(ore_types[i], ore_positions[i]);
        _ore_manager->add_ores(ores);
        _ore_manager->build();

        _is_filled.store(true);
        return true;
    }

    std::pair<ChunkVertex*, size_t> vertices() {
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
                Rect src = {
                    static_cast<int>((clip.x * TILE_ORIGINAL_WIDTH) + ((clip.x + 1) * TILE_PADDING)),
                    static_cast<int>((clip.y * TILE_ORIGINAL_HEIGHT) + ((clip.y + 1) * TILE_PADDING)),
                    TILE_ORIGINAL_WIDTH, TILE_ORIGINAL_HEIGHT
                };

                // Calculate world position for this tile
                float tile_x = x * TILE_WIDTH;
                float tile_y = y * TILE_HEIGHT;

                glm::vec2 _positions[] = {
                    {tile_x, tile_y},                            // Top-left
                    {tile_x + TILE_WIDTH, tile_y},               // Top-right
                    {tile_x + TILE_WIDTH, tile_y + TILE_HEIGHT}, // Bottom-right
                    {tile_x, tile_y + TILE_HEIGHT},              // Bottom-left
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

                static uint16_t _indices[] = {0, 1, 2, 2, 3, 0};
                for (int i = 0; i < 6; i++) {
                    ChunkVertex *v = &vertices[vertex_index + i];
                    v->position = _positions[_indices[i]];
                    v->texcoord = _texcoords[_indices[i]];
                }
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

    void draw_ores() {
        if (_ore_manager->is_dirty())
            _ore_manager->build();
        _ore_manager->draw();
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

    static uint64_t id(int _x, int _y) {
        // Map (x,y) coordinates to a unique ID
#define _INDEX(I) (abs((I) * 2) - ((I) > 0 ? 1 : 0))
        int x = _INDEX(_x), y = _INDEX(_y);
        return x >= y ? x * x + x + y : x + y * y;
    }

    static std::pair<int, int> unindex(uint64_t id_value) {
        // First, we need to find which (x,y) pair in the _INDEX space maps to this id
        // The formula is: id = x >= y ? x*x + x + y : x + y*y
        // We need to reverse this process
        // Find the "diagonal" we're on (similar to Cantor pairing function)
        uint64_t w = static_cast<uint64_t>(floor(sqrt(static_cast<double>(id_value))));
        uint64_t t = w * w;
        int ix, iy;
        if (id_value < t + w) {
            // We're in the lower triangle: x + y*y = id_value, where y = w
            ix = static_cast<int>(id_value - t);
            iy = static_cast<int>(w);
        } else {
            // We're in the upper triangle: x*x + x + y = id_value, where x = w
            ix = static_cast<int>(w);
            iy = static_cast<int>(id_value - t - w);
        }
        // Now convert from _INDEX space back to regular coordinates
#define _UNINDEX(I) ((I) == 0 ? 0 : ((I) % 2 == 1) ? ((I) + 1) / 2 : -((I) / 2))
        return {_UNINDEX(ix), _UNINDEX(iy)};
    }

    static Rect bounds(int _x, int _y) {
        return {
            .x = _x * CHUNK_WIDTH * TILE_WIDTH,
            .y = _y * CHUNK_HEIGHT * TILE_HEIGHT,
            .w = CHUNK_WIDTH * TILE_WIDTH,
            .h = CHUNK_HEIGHT * TILE_HEIGHT
        };
    }

    uint64_t id() const { return Chunk::id(_x, _y); }
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

class ChunkManager {
    std::unordered_map<uint64_t, Chunk*> _chunks;
    mutable std::shared_mutex _chunks_lock;
    JobQueue<std::pair<int, int>> _create_chunk_queue;
    ThreadSafeSet<uint64_t> _chunks_being_created;
    JobQueue<Chunk*> _build_chunk_queue;
    ThreadSafeSet<uint64_t> _chunks_being_built;
    ThreadSafeSet<uint64_t> _chunks_being_destroyed;

    Camera *_camera;
    Texture *_tilemap;
    sg_shader _shader;
    sg_pipeline _pipeline;
    sg_pipeline _ore_pipeline;

    void ensure_chunk(int x, int y, bool priority) {
        uint64_t idx = Chunk::id(x, y);

        // Check if chunk already exists or is being processed
        {
            std::shared_lock<std::shared_mutex> chunks_lock(_chunks_lock);
            if (_chunks.find(idx) != _chunks.end())
                return;
        }

        // Check and insert atomically using ThreadSafeSet methods
        if (_chunks_being_created.contains(idx) ||
            _chunks_being_built.contains(idx))
            return;

        // Try to mark as being created
        if (!_chunks_being_created.insert(idx))
            return; // Another thread already marked it

        if (priority)
            _create_chunk_queue.enqueue_priority({x, y});
        else
            _create_chunk_queue.enqueue({x, y});
    }

    void update_chunk(Chunk *chunk, const Rect &camera_bounds, const Rect &max_bounds) {
        if (!chunk->is_ready())
            return;

        ChunkVisibility last_visibility = chunk->visibility();
        Rect chunk_bounds = chunk->bounds();
        ChunkVisibility new_visibility = !max_bounds.intersects(chunk_bounds) ? ChunkVisibility::OutOfSign :
                                         camera_bounds.intersects(chunk_bounds) ? ChunkVisibility::Visible :
                                         ChunkVisibility::Occluded;
        chunk->set_visibility(new_visibility);
        if (new_visibility != last_visibility) {
            std::cout << fmt::format("Chunk at ({}, {}) visibility changed from {} to {}\n",
                                     chunk->x(), chunk->y(),
                                     Chunk::visibility_to_string(last_visibility),
                                     Chunk::visibility_to_string(new_visibility));
            if (new_visibility == ChunkVisibility::OutOfSign) {
                _chunks_being_destroyed.insert(chunk->id());
                chunk->mark_destroyed();
            }
        }
        // TODO: Add timer to Occulded chunks, if not visible for x seconds mark for deletion
        //       If chunk becomes visible again it will save time reloading it
    }

public:
    ChunkManager(Camera *camera): _camera(camera),
    _create_chunk_queue([&](std::pair<int, int> coords) {
        auto [x, y] = coords;
        uint64_t idx = Chunk::id(x, y);
        Chunk *chunk = new Chunk(x, y, _camera, _tilemap);

        {
            std::unique_lock<std::shared_mutex> lock(_chunks_lock);
            std::cout << fmt::format("New chunk created at ({}, {})\n", x, y);
            _chunks[idx] = chunk;
            _chunks_being_created.erase(idx);  // Remove from being created set
        }

        chunk->fill();
        std::cout << fmt::format("Chunk at ({}, {}) finished filling\n", x, y);

        {
            std::lock_guard<std::shared_mutex> lock(_chunks_lock);
            _chunks_being_built.insert(idx);  // Mark as being built
        }
        _build_chunk_queue.enqueue(chunk);
    }),
    _build_chunk_queue([&](Chunk *chunk) {
        chunk->build();
        std::cout << fmt::format("Chunk at ({}, {}) finished building\n", chunk->x(), chunk->y());

        // Remove from being built set after successful build
        uint64_t idx = chunk->id();
        _chunks_being_built.erase(idx);
    }) {
        _shader = sg_make_shader(basic_shader_desc(sg_query_backend()));
        sg_pipeline_desc desc = {
            .shader = _shader,
            .layout = {
                .buffers[0].stride = sizeof(ChunkVertex),
                .attrs = {
                    [ATTR_basic_position].format = SG_VERTEXFORMAT_FLOAT2,
                    [ATTR_basic_texcoord].format = SG_VERTEXFORMAT_FLOAT2
                }
            },
            .depth = {
                .pixel_format = SG_PIXELFORMAT_DEPTH,
                .compare = SG_COMPAREFUNC_LESS_EQUAL,
                .write_enabled = true
            },
            .cull_mode = SG_CULLMODE_BACK,
            .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8
        };
        _pipeline = sg_make_pipeline(&desc);
        desc.colors[0] = {
            .pixel_format = SG_PIXELFORMAT_RGBA8,
            .blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .src_factor_alpha = SG_BLENDFACTOR_ONE,
                .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
            }
        };
        _ore_pipeline = sg_make_pipeline(&desc);
        auto tilemap = $Assets.get<Texture>("assets/tilemap.exploded.png");
        if (!tilemap.has_value())
            throw std::runtime_error("Failed to load tilemap texture");
        _tilemap = tilemap.value();
    }

    ~ChunkManager() {
        if (sg_query_shader_state(_shader) == SG_RESOURCESTATE_VALID)
            sg_destroy_shader(_shader);
        if (sg_query_pipeline_state(_pipeline) == SG_RESOURCESTATE_VALID)
            sg_destroy_pipeline(_pipeline);
        if (sg_query_pipeline_state(_ore_pipeline) == SG_RESOURCESTATE_VALID)
            sg_destroy_pipeline(_ore_pipeline);
        {
            std::unique_lock<std::shared_mutex> lock(_chunks_lock);
            for (auto& [id, chunk] : _chunks)
                if (chunk != nullptr)
                    delete chunk;
            _chunks.clear();
        }
    }

    void update_chunks() {
        Rect max_bounds = _camera->max_bounds();
        Rect bounds = _camera->bounds();

        // Collect chunks to update without holding the lock
        std::vector<Chunk*> chunks_to_update;
        {
            std::shared_lock<std::shared_mutex> lock(_chunks_lock);
            chunks_to_update.reserve(_chunks.size());
            for (const auto& [id, chunk] : _chunks)
                chunks_to_update.push_back(chunk);
        }

        // Update chunks without holding the lock
        for (auto chunk : chunks_to_update)
            update_chunk(chunk, bounds, max_bounds);
    }

    void scan_for_chunks() {
        Rect max_bounds = _camera->max_bounds();
        Rect bounds = _camera->bounds();
        glm::vec2 tl = glm::vec2(max_bounds.x, max_bounds.y);
        glm::vec2 br = glm::vec2(max_bounds.x + max_bounds.w, max_bounds.y + max_bounds.h);
        glm::vec2 tl_chunk = _camera->world_to_chunk(tl);
        glm::vec2 br_chunk = _camera->world_to_chunk(br);
        for (int y = (int)tl_chunk.y; y <= (int)br_chunk.y; y++)
            for (int x = (int)tl_chunk.x; x <= (int)br_chunk.x; x++) {
                Rect chunk_bounds = Chunk::bounds(x, y);
                if (chunk_bounds.intersects(max_bounds))
                    ensure_chunk(x, y, chunk_bounds.intersects(bounds));
            }
    }

    void release_chunks() {
        std::vector<uint64_t> chunks_to_destroy;
        std::vector<Chunk*> chunks_to_delete;
        {
            std::unique_lock<std::shared_mutex> lock(_chunks_lock);
            // Iterate through chunks and check if they're marked for destruction
            for (auto it = _chunks.begin(); it != _chunks.end();) {
                uint64_t chunk_id = it->first;
                Chunk* chunk = it->second;

                if (_chunks_being_destroyed.contains(chunk_id)) {
                    std::cout << fmt::format("Releasing chunk at ({}, {})\n", chunk->x(), chunk->y());
                    chunks_to_delete.push_back(chunk);
                    chunks_to_destroy.push_back(chunk_id);
                    it = _chunks.erase(it);
                } else
                    ++it;
            }
        }

        // Delete chunks outside of the lock
        for (Chunk* chunk : chunks_to_delete)
            delete chunk;
        // Remove from destroyed set
        for (uint64_t chunk_id : chunks_to_destroy)
            _chunks_being_destroyed.erase(chunk_id);
    }

    void draw_chunks() {
        // Collect valid chunks without holding the lock for too long
        std::vector<std::pair<uint64_t, Chunk*>> valid_chunks;
        {
            std::shared_lock<std::shared_mutex> lock(_chunks_lock);
            valid_chunks.reserve(_chunks.size());
            for (const auto& [id, chunk] : _chunks)
                if (chunk != nullptr && !_chunks_being_destroyed.contains(id))
                    valid_chunks.emplace_back(id, chunk);
        }

        // Draw chunks first
        sg_apply_pipeline(_pipeline);
        bool force_update_mvp = _camera->is_dirty();
        for (const auto& [id, chunk] : valid_chunks) {
            // Double-check chunk is still valid (avoid race condition)
            if (_chunks_being_destroyed.contains(id))
                continue;
            chunk->draw(force_update_mvp);
        }

        // Draw ores
        sg_apply_pipeline(_ore_pipeline);
        for (const auto& [id, chunk] : valid_chunks)
            chunk->draw_ores();
    }
};
