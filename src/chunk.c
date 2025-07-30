//
//  chunk.c
//  rpg
//
//  Created by George Watson on 29/07/2025.
//

#include "chunk.h"
#include "default.glsl.h"

bool chunk_create(struct chunk *c, int x, int y, enum chunk_state state) {
    assert(c != NULL);
    memset(c, 0, sizeof(struct chunk));
    c->x = x;
    c->y = y;
    c->state = state;
    c->dirty = true;
    return pthread_mutex_init(&c->lock, NULL) == 0;
}

void chunk_destroy(struct chunk *c) {
    if (!c)
        return;
    pthread_mutex_destroy(&c->lock);
    if (sg_query_buffer_state(c->bind.vertex_buffers[0]) == SG_RESOURCESTATE_VALID)
        sg_destroy_buffer(c->bind.vertex_buffers[0]);
    memset(c, 0, sizeof(struct chunk));
}

union tile* chunk_tile(struct chunk *c, int x, int y) {
    return x < 0 || x >= CHUNK_WIDTH ||
           y < 0 || y >= CHUNK_HEIGHT ? NULL :
           &c->grid[y * CHUNK_WIDTH + x];
}

static uint8_t _calc_bitmask(uint8_t neighbours[9]) {
#define CHECK_CORNER(N, A, B) \
    neighbours[(N)] = !neighbours[(A)] || !neighbours[(B)] ? 0 : neighbours[(N)];
    CHECK_CORNER(0, 1, 3);
    CHECK_CORNER(2, 1, 5);
    CHECK_CORNER(6, 7, 3);
    CHECK_CORNER(8, 7, 5);
#undef CHECK_CORNER
    uint8_t result = 0;
    for (int y = 0, n = 0; y < 3; y++)
        for (int x = 0; x < 3; x++)
            if (!(y == 1 && x == 1))
                result += (neighbours[y * 3 + x] << n++);
    return result;
}

static uint8_t _solid(struct chunk *chunk, int x, int y, int oob) {
    return x < 0 || y < 0 || x >= CHUNK_WIDTH || y >= CHUNK_HEIGHT ? oob : chunk->grid[y * CHUNK_WIDTH + x].solid ? 1 : 0;
}

static uint8_t _bitmask(struct chunk *chunk, int cx, int cy, int oob) {
    uint8_t neighbours[9] = {0};
    for (int x = -1; x < 2; x++)
        for (int y = -1; y < 2; y++)
            neighbours[(y+1)*3+(x+1)] = !x && !y ? 0 : _solid(chunk, cx+x, cy+y, oob);
    return _calc_bitmask(neighbours);
}

void chunk_fill(struct chunk *c) {
    assert(c != NULL);

    uint8_t _grid[CHUNK_SIZE];
    cellular_automata(CHUNK_WIDTH, CHUNK_HEIGHT,
                      CHUNK_FILL_CHANCE, CHUNK_SMOOTH_ITERATIONS,
                      CHUNK_SURVIVE, CHUNK_STARVE,
                      _grid);
    chunk_each(c, _grid, ^(int x, int y, union tile *tile, void *userdata) {
        uint8_t *_grid = (uint8_t*)userdata;
        tile->solid = _grid[y * CHUNK_WIDTH + x];
        tile->bitmask = tile->solid ? _bitmask(c, x, y, 0) : 0;
    });
}

void chunk_each(struct chunk *c, void *userdata, void(^fn)(int x, int y, union tile *tile, void *userdata)) {
    assert(c != NULL && fn != NULL);
    pthread_mutex_lock(&c->lock);
    for (int y = 0; y < CHUNK_HEIGHT; y++)
        for (int x = 0; x < CHUNK_WIDTH; x++) {
            union tile *tile = &c->grid[y * CHUNK_WIDTH + x];
            fn(x, y, tile, userdata);
        }
    pthread_mutex_unlock(&c->lock);
}

static const HMM_Vec2 Autotile3x3Simplified[256] = {
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

void chunk_build(struct chunk *c, struct texture *texture) {
    if (sg_query_buffer_state(c->bind.vertex_buffers[0]) == SG_RESOURCESTATE_VALID)
        sg_destroy_buffer(c->bind.vertex_buffers[0]);
    c->dirty = false;

    float hw = framebuffer_width() / 2.f;
    float hh = framebuffer_height() / 2.f;
    static const size_t mem_size = CHUNK_SIZE * 6 * sizeof(struct chunk_vertex);
    struct chunk_vertex *vertices = malloc(mem_size);
    memset(vertices, 0, mem_size);
    for (int x = 0; x < CHUNK_WIDTH; x++)
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            union tile *tile = &c->grid[y * CHUNK_WIDTH + x];
            if (!tile->solid)
                continue;
            tile->bitmask = tile->solid ? _bitmask(c, x, y, 0) : 0;

            HMM_Vec2 clip = Autotile3x3Simplified[tile->bitmask];
            struct rect src = {
                (clip.X * TILE_WIDTH) + ((clip.X + 1) * TILE_PADDING),
                (clip.Y * TILE_HEIGHT) + ((clip.Y + 1) * TILE_PADDING),
                TILE_WIDTH, TILE_HEIGHT
            };

            float hqw = TILE_WIDTH / 2.f;
            float hqh = TILE_HEIGHT / 2.f;
            HMM_Vec2 _positions[] = {
                {hw - hqw, hh - hqh}, // Top-left
                {hw + hqw, hh - hqh}, // Top-right
                {hw + hqw, hh + hqh}, // Bottom-right
                {hw - hqw, hh + hqh}, // Bottom-left
            };

            float iw = 1.f / texture->width;
            float ih = 1.f / texture->height;
            float tl = src.X * iw;
            float tt = src.Y * ih;
            float tr = (src.X + src.W) * iw;
            float tb = (src.Y + src.H) * ih;
            HMM_Vec2 _texcoords[4] = {
                {tl, tt}, // top left
                {tr, tt}, // top right
                {tr, tb}, // bottom right
                {tl, tb}, // bottom left
            };

            uint16_t indices[] = { 0, 1, 2, 2, 3, 0 };
            for (int i = 0; i < 6; i++) {
                struct chunk_vertex *v = &vertices[(y * CHUNK_WIDTH + x) * 6 + i];
                HMM_Vec2 offset = HMM_V2(x * TILE_WIDTH, y * TILE_HEIGHT);
                v->position = HMM_AddV2(_positions[indices[i]], offset);
                v->texcoord = _texcoords[indices[i]];
            }
        }

    c->bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .data = {
            .ptr = vertices,
            .size = mem_size,
        },
    });
    free(vertices);
}

void chunk_draw(struct chunk *c, struct texture *texture, struct camera *camera) {
    assert(c != NULL && camera != NULL);
    if (texture) {
        c->bind.images[IMG_tex] = texture->image;
        c->bind.samplers[SMP_smp] = texture->sampler;
    }

    if (c->dirty)
        chunk_build(c, texture);

    if (camera->dirty)
        c->mvp = HMM_MulM4(camera_mvp(camera, sapp_width(), sapp_height()),
                           HMM_Translate(HMM_V3(c->x * CHUNK_WIDTH * TILE_WIDTH,
                                                c->y * CHUNK_HEIGHT * TILE_HEIGHT,
                                                0)));

    sg_apply_bindings(&c->bind);
    vs_params_t params;
    memcpy(params.mvp, &c->mvp.Elements, sizeof(float) * 16);
    sg_apply_uniforms(UB_vs_params, &SG_RANGE(params));
    sg_draw(0, CHUNK_SIZE * 6, 1);
}

HMM_Vec2 camera_screen_to_chunk(struct camera *cam, HMM_Vec2 screen_pos, int width, int height) {
    HMM_Vec2 world = camera_screen_to_world(cam, screen_pos, width, height);
    return HMM_V2(floor(world.X / (CHUNK_WIDTH * TILE_WIDTH)),
                  floor(world.Y / (CHUNK_HEIGHT * TILE_HEIGHT)));
}

HMM_Vec2 camera_screen_to_tile(struct camera *cam, HMM_Vec2 screen_pos, int width, int height) {
    HMM_Vec2 world = camera_screen_to_world(cam, screen_pos, width, height);
    HMM_Vec2 chunk = HMM_V2(world.X / (CHUNK_WIDTH * TILE_WIDTH),
                            world.Y / (CHUNK_HEIGHT * TILE_HEIGHT));
    HMM_Vec2 tile = HMM_V2(fmod(world.X, CHUNK_WIDTH * TILE_WIDTH) / TILE_WIDTH,
                           fmod(world.Y, CHUNK_HEIGHT * TILE_HEIGHT) / TILE_HEIGHT);
    HMM_Vec2 final = HMM_V2((int)(chunk.X * CHUNK_WIDTH + tile.X) % CHUNK_WIDTH,
                            (int)(chunk.Y * CHUNK_HEIGHT + tile.Y) % CHUNK_HEIGHT);
    return HMM_V2(final.X < 0 ? CHUNK_WIDTH + final.X : final.X,
                  final.Y < 0 ? CHUNK_HEIGHT + final.Y : final.Y);
}

struct rect chunk_bounds_ex(int x, int y) {
    static const int chunkW = CHUNK_WIDTH * TILE_WIDTH;
    static const int chunkH = CHUNK_HEIGHT * TILE_HEIGHT;
    return (struct rect){chunkW * x, chunkH * y, chunkW, chunkH};
}

struct rect chunk_bounds(struct chunk *chunk) {
    return chunk_bounds_ex(chunk->x, chunk->y);
}
