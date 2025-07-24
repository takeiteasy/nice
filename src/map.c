//
//  map.c
//  rpg
//
//  Created by George Watson on 23/07/2025.
//

#include "map.h"
#include "default.glsl.h"

struct chunk_vertex {
    vec2 position;
    vec2 texcoord;
    vec4 color;
};

static inline int chunk_index(int x, int y) {
#define INDEX(I) (abs((I) * 2) - ((I) > 0 ? 1 : 0))
    int _x = INDEX(x), _y = INDEX(y);
    return _x >= _y ? _x * _x + _x + _y : _x + _y * _y;
}

void map_create(struct map *map, const char *tilemap) {
    assert(map != NULL || tilemap != NULL);

    map->chunks = table();
    // TODO: Get CPU core count and use that instead
    thrd_pool_create(8, &map->pool);
    map->delete_queue = NULL;

    map->pass_action = (sg_pass_action) {
        .colors[0] = {
            .load_action = SG_LOADACTION_CLEAR,
            .clear_value = {0.0f, 0.0f, 0.0f, 1.0f}
        }
    };

    map->pip = sg_make_pipeline(&(sg_pipeline_desc) {
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .shader = sg_make_shader(default_program_shader_desc(sg_query_backend())),
        .layout = {
            .buffers[0].stride = sizeof(struct chunk_vertex),
            .attrs = {
                [ATTR_default_program_position].format = SG_VERTEXFORMAT_FLOAT2,
                [ATTR_default_program_texcoord].format = SG_VERTEXFORMAT_FLOAT2,
                [ATTR_default_program_color].format    = SG_VERTEXFORMAT_FLOAT4
            }
        },
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true
        }
    });

    assert(texture_load_path(&map->tilemap, tilemap));
}

void map_destroy(struct map *map) {
    if (!map)
        return;
    table_each(&map->chunks, NULL, ^(table_t *table, const char *key, table_entry_t *entry, void *userdata) {
        struct chunk *c = (struct chunk*)entry->value;
        // TODO: Export chunk to disk if needed
        chunk_destroy(c);
    });
    table_free(&map->chunks);
    thrd_pool_destroy(&map->pool);
    if (map->delete_queue)
        garry_free(map->delete_queue);
    if (sg_query_image_state(map->tilemap.image) == SG_RESOURCESTATE_VALID)
        texture_destroy(&map->tilemap);
}

static struct chunk* map_chunk_new(struct map *map, int x, int y) {
    struct chunk *c = malloc(sizeof(struct chunk));
    if (!chunk_create(c, x, y, CHUNK_STATE_DORMANT)) {
        free(c);
        return NULL;
    }

    uint8_t grid[CHUNK_SIZE];
    cellular_automata(CHUNK_WIDTH, CHUNK_HEIGHT,
                      CHUNK_FILL_CHANCE, CHUNK_SMOOTH_ITERATIONS,
                      CHUNK_SURVIVE, CHUNK_STARVE,
                      grid);
    /* IDEA:
     - loop through edges
     - if distance to edge is 0, set to 0 (empty)
     - if distance 1>=d>=5, chance to survive = 100% - (n * d) (n = 10?)
     - avoids ugly edge issues between chunks
     - cave system more together like ant farm
     - no need to lookup neighbouring chunks
     */
    chunk_each(c, grid, ^(int x, int y, union tile *tile, void *userdata) {
        tile->value = x == 0 || x == CHUNK_WIDTH  - 1 ||
                      y == 0 || y == CHUNK_HEIGHT - 1 ?
                      1 : ((uint8_t*)userdata)[y * CHUNK_WIDTH + x];
    });

    if (!table_set(&map->chunks, index, c)) {
        chunk_destroy(c);
        free(c);
        return NULL;
    }
    return c;
}

struct chunk* map_chunk(struct map *map, int x, int y) {
    assert(map != NULL);
    int index = chunk_index(x, y);
    if (table_has(&map->chunks, index)) {
        struct chunk *chunk = NULL;
        return table_get(&map->chunks, index, &chunk) ? chunk : NULL;
    }
    // TODO: Attempt to find chunk on disk
    return map_chunk_new(map, x, y);
}
