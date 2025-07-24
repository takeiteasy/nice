//
//  chunk.c
//  rpg
//
//  Created by George Watson on 23/07/2025.
//

#include "chunk.h"

bool chunk_create(struct chunk *c, int x, int y, enum chunk_state state) {
    assert(c != NULL);
    c->x = x;
    c->y = y;
    c->state = state;
    return pthread_mutex_init(&c->lock, NULL) == 0;
}

void chunk_destroy(struct chunk *c) {
    if (!c)
        return;
    pthread_mutex_destroy(&c->lock);
    memset(c, 0, sizeof(struct chunk));
}

union tile* chunk_tile(struct chunk *c, int x, int y) {
    return x < 0 || x >= CHUNK_WIDTH ||
           y < 0 || y >= CHUNK_HEIGHT ? NULL :
           &c->grid[y * CHUNK_WIDTH + x];
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
