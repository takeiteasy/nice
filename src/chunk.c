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

void chunk_fill(struct chunk *c) {
    assert(c != NULL);
    uint8_t grid[CHUNK_SIZE];
    cellular_automata(CHUNK_WIDTH, CHUNK_HEIGHT,
                      CHUNK_FILL_CHANCE, CHUNK_SMOOTH_ITERATIONS,
                      CHUNK_SURVIVE, CHUNK_STARVE,
                      grid);
    pthread_mutex_lock(&c->lock);
    for (int x = 0; x < CHUNK_WIDTH; x++)
        for (int y = 0; y < CHUNK_HEIGHT; y++)
            c->grid[y * CHUNK_WIDTH + x].solid = x == 0 || x == CHUNK_WIDTH  - 1 ||
                                                 y == 0 || y == CHUNK_HEIGHT - 1 ?
                                                 1 : grid[y * CHUNK_WIDTH + x];
    pthread_mutex_unlock(&c->lock);
}
