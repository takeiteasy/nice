//
//  map.c
//  rpg
//
//  Created by George Watson on 30/07/2025.
//

#include "map.h"
#include "default.glsl.h"

#define _INDEX(I) (abs((I) * 2) - ((I) > 0 ? 1 : 0))

static inline int chunk_index_ex(int x, int y) {
    int _x = _INDEX(x), _y = _INDEX(y);
    return _x >= _y ? _x * _x + _x + _y : _x + _y * _y;
}

static inline int chunk_index(struct chunk *chunk) {
    return chunk_index_ex(chunk->x, chunk->y);
}

static inline bool rect_intersect(struct rect a, struct rect b) {
    return b.X <= a.X + a.W &&
           b.X + b.W >= a.X &&
           b.Y <= a.Y + a.H &&
           b.Y + b.H >= a.Y;
}

bool map_create(struct map *map) {
    assert(map != NULL);
    assert(texture_load_path(&map->tilemap, "assets/tilemap.exploded.png"));

    map->chunks = table();
    map->camera = camera_create(0, 0, 1, 0);
    map->shader = sg_make_shader(default_program_shader_desc(sg_query_backend()));
    assert(sg_query_shader_state(map->shader) == SG_RESOURCESTATE_VALID);

    map->pipeline = sg_make_pipeline(&(sg_pipeline_desc) {
        .shader = map->shader,
        .layout = {
            .buffers[0].stride = sizeof(struct chunk_vertex),
            .attrs = {
                [ATTR_default_program_position].format = SG_VERTEXFORMAT_FLOAT2,
                [ATTR_default_program_texcoord].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .depth = {
            .pixel_format = SG_PIXELFORMAT_DEPTH,
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true
        },
        .cull_mode = SG_CULLMODE_BACK,
        .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8
    });

    assert(sg_query_pipeline_state(map->pipeline) == SG_RESOURCESTATE_VALID);
    assert(thrd_pool_create(8, &map->pool));
    assert(thrd_queue_create(&map->delete_queue));
    return true;
}

void map_destroy(struct map *map) {
    if (!map)
        return;
    sg_destroy_shader(map->shader);
    sg_destroy_pipeline(map->pipeline);
    texture_destroy(&map->tilemap);
    table_each(&map->chunks, NULL, ^(table_t *table, const char *key, table_entry_t *entry, void *userdata) {
        struct chunk *c = (struct chunk*)entry->value;
        // TODO: Export chunk to disk if needed
        chunk_destroy(c);
        free(c);
    });
    table_free(&map->chunks);
    thrd_pool_destroy(&map->pool);
    thrd_queue_destroy(&map->delete_queue, NULL);
}

struct chunk* map_chunk(struct map *map, int x, int y, bool ensure) {
    assert(map != NULL);
    int index = chunk_index_ex(x, y);
    struct chunk *chunk = NULL;
    if (table_get(&map->chunks, index, &chunk))
        return chunk;
    if (!ensure)
        return NULL;

    // TODO: Attempt to find chunk on disk
    assert(chunk = malloc(sizeof(struct chunk)));
    if (!chunk_create(chunk, x, y, CHUNK_STATE_DORMANT)) {
        free(chunk);
        return NULL;
    }
    if (!table_set(&map->chunks, index, chunk)) {
        chunk_destroy(chunk);
        free(chunk);
        return NULL;
    }
    chunk_fill(chunk);
    return chunk;
}

void map_draw(struct map *map) {
    assert(map != NULL);

    sg_apply_pipeline(map->pipeline);
    table_each(&map->chunks, NULL, ^(table_t *table, const char *key, table_entry_t *entry, void *userdata) {
        chunk_draw((struct chunk*)entry->value, &map->tilemap, &map->camera);
    });
}

void map_update(struct map *map) {

}
