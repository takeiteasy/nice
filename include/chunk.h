//
//  chunk.h
//  rpg
//
//  Created by George Watson on 29/07/2025.
//

#pragma once

#ifndef CHUNK_WIDTH
#define CHUNK_WIDTH  64
#endif
#ifndef CHUNK_HEIGHT
#define CHUNK_HEIGHT 64
#endif
#define CHUNK_SIZE ((CHUNK_WIDTH)*(CHUNK_HEIGHT))

#ifndef CHUNK_FILL_CHANCE
#define CHUNK_FILL_CHANCE 40
#endif
#ifndef CHUNK_SMOOTH_ITERATIONS
#define CHUNK_SMOOTH_ITERATIONS 5
#endif
#ifndef CHUNK_SURVIVE
#define CHUNK_SURVIVE 4
#endif
#ifndef CHUNK_STARVE
#define CHUNK_STARVE 3
#endif

#ifndef TILE_WIDTH
#define TILE_WIDTH 8
#endif
#ifndef TILE_HEIGHT
#define TILE_HEIGHT 8
#endif
#ifndef TILE_PADDING
#define TILE_PADDING 4
#endif

#include "pthread_shim.h"
#include "camera.h"
#include "texture.h"
#include "rng.h"
#include "sokol/sokol_app.h"

union tile {
    struct {
        uint8_t bitmask;
        uint8_t visible;
        uint8_t solid;
        uint8_t extra;
    };
    uint32_t value;
};

enum chunk_state {
    CHUNK_STATE_UNLOAD = 0,
    CHUNK_STATE_DORMANT,
    CHUNK_STATE_ACTIVE
};

struct chunk_vertex {
    HMM_Vec2 position;
    HMM_Vec2 texcoord;
    HMM_Vec4 color;
};

struct chunk {
    int x, y;
    enum chunk_state state;
    union tile grid[CHUNK_SIZE];
    pthread_mutex_t lock;
    bool dirty;
    sg_bindings bind;
    HMM_Mat4 mvp;
};

bool chunk_create(struct chunk *c, int x, int y, enum chunk_state state);
void chunk_destroy(struct chunk *c);
void chunk_fill(struct chunk *c);
union tile* chunk_tile(struct chunk *c, int x, int y);
void chunk_each(struct chunk *c, void *userdata, void(^fn)(int x, int y, union tile *tile, void *userdata));
void chunk_draw(struct chunk *c, struct texture *texture, struct camera *camera);
