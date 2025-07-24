//
//  map.c
//  rpg
//
//  Created by George Watson on 23/07/2025.
//

#include "map.h"

#define INDEX(I) (abs((I) * 2) - ((I) > 0 ? 1 : 0))

static inline int chunk_index(int x, int y) {
    int _x = INDEX(x), _y = INDEX(y);
    return _x >= _y ? _x * _x + _x + _y : _x + _y * _y;
}

void map_create(struct map *map) {
    assert(map != NULL);
    map->chunks = table();
    thrd_pool_create(8, &map->pool);
    map->delete_queue = NULL;
}

void map_destroy(struct map *map) {
    if (!map)
        return;
    table_each(&map->chunks, NULL,
               ^(table_t *table, const char *key, table_entry_t *entry, void *userdata) {
        struct chunk *c = (struct chunk*)entry->value;
        // TODO: Export chunk to disk if needed
        chunk_destroy(c);
    });
    table_free(&map->chunks);
    thrd_pool_destroy(&map->pool);
    if (map->delete_queue)
        garry_free(map->delete_queue);
}

struct chunk* map_chunk(struct map *map, int x, int y) {
    assert(map != NULL);
    int index = chunk_index(x, y);
    if (table_has(&map->chunks, index)) {
        struct chunk *chunk = NULL;
        return table_get(&map->chunks, index, &chunk) ? chunk : NULL;
    }
    // TODO: Attempt to find chunk on disk
    struct chunk *chunk = malloc(sizeof(struct chunk));
    if (!chunk_create(chunk, x, y, CHUNK_STATE_DORMANT)) {
        free(chunk);
        return NULL;
    }
    if (!table_set(&map->chunks, index, chunk)) {
        chunk_destroy(chunk);
        free(chunk);
        return NULL;
    }
    return chunk;
}
