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

static struct chunk* add_chunk(struct map *map, int x, int y);

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

    map->delete_queue = NULL;
    map->delete_queue_size = 0;
    assert(pool_create(&map->worker, 8));

    return true;
}

void map_destroy(struct map *map) {
    if (!map)
        return;
    pool_destroy(&map->worker);
    sg_destroy_shader(map->shader);
    sg_destroy_pipeline(map->pipeline);
    texture_destroy(&map->tilemap);
    if (map->delete_queue)
        free(map->delete_queue);
    table_each(&map->chunks, NULL, ^(table_t *table, const char *key, table_entry_t *entry, void *userdata) {
        struct chunk *c = (struct chunk*)entry->value;
        // TODO: Export chunk to disk if needed
        chunk_destroy(c);
        free(c);
    });
    table_free(&map->chunks);
}

static const char* chunk_state_str(enum chunk_state state) {
    switch (state) {
        case CHUNK_STATE_UNLOAD:
            return "UNLOAD";
        case CHUNK_STATE_DORMANT:
            return "DORMANT";
        case CHUNK_STATE_ACTIVE:
            return "ACTIVE";
        default: return "UNKNOWN";
    }
}

#define _RADIANS(X) ((X) * (M_PI / 180.0f))
#define _DEGREES(X) ((X) * (180.0f / M_PI))

enum cardinal {
    EAST = 0,
    SOUTH_EAST,
    SOUTH,
    SOUTH_WEST,
    WEST,
    NORTH_WEST,
    NORTH,
    NORTH_EAST
};

static enum cardinal cardinal(int x1, int y1, int x2, int y2) {
    float l1 = _RADIANS((float)x1);
    float l2 = _RADIANS((float)x2);
    float dl = _RADIANS((float)y2 - (float)y1);
    return (int)round((((int)_DEGREES(atan2(sin(dl) * cos(l2), cos(l1) * sin(l2) - (sin(l1) * cos(l2) * cos(dl)))) + 360) % 360) / 45.f);
}

static HMM_Vec2 cardinal_delta(struct chunk *a, struct chunk *b) {
    switch (cardinal(a->x, a->y, b->x, b->y)) {
        default:
            return HMM_V2(0, 0);
        case EAST:
            return HMM_V2(-1, 0);
        case SOUTH_EAST:
            return HMM_V2(-1, -1);
        case SOUTH:
            return HMM_V2(0, -1);
        case SOUTH_WEST:
            return HMM_V2(1, -1);
        case WEST:
            return HMM_V2(1, 0);
        case NORTH_WEST:
            return HMM_V2(1, 1);
        case NORTH:
            return HMM_V2(0, 1);
        case NORTH_EAST:
            return HMM_V2(-1, 1);
    }
}

static struct chunk* add_chunk(struct map *map, int x, int y) {
    struct chunk *chunk = malloc(sizeof(struct chunk));
    if (!chunk_create(chunk, x, y, CHUNK_STATE_DORMANT)) {
        free(chunk);
        return NULL;
    }
    if (!table_set(&map->chunks, chunk_index(chunk), chunk)) {
        chunk_destroy(chunk);
        free(chunk);
        chunk = NULL;
    }
    chunk_fill(chunk);
    printf("CHUNK ADDED (%d, %d) (%s)\n", x, y, chunk_state_str(chunk->state));
    return chunk;
}

struct chunk* map_chunk(struct map *map, int x, int y, bool ensure) {
    assert(map != NULL);
    int index = chunk_index_ex(x, y);
    struct chunk *chunk = NULL;
    table_get(&map->chunks, index, &chunk);
    if (chunk != NULL)
        return chunk;
    if (!ensure)
        return NULL;
    // TODO: Attempt to find chunk on disk
    return add_chunk(map, x, y);
}

static void draw_chunks(struct map *map) {
    sg_apply_pipeline(map->pipeline);
    table_each(&map->chunks, NULL, ^(table_t *table, const char *key, table_entry_t *entry, void *userdata) {
        chunk_draw((struct chunk*)entry->value, &map->tilemap, &map->camera);
    });
}

static bool update_chunk_state(struct chunk *chunk, struct camera *camera) {
    struct rect chunk_b = chunk_bounds(chunk);
    enum chunk_state state = chunk->state;
    if (!rect_intersect(camera_bounds_ex(camera->position.X, camera->position.Y, MIN_ZOOM), chunk_b))
        chunk->state = CHUNK_STATE_UNLOAD;
    else
        chunk->state = rect_intersect(camera_bounds(camera), chunk_b) ? CHUNK_STATE_ACTIVE : CHUNK_STATE_DORMANT;
    return state != chunk->state;
}

static void find_chunks(struct map *map) {
    struct rect camera_b = camera_bounds_ex(map->camera.position.X, map->camera.position.Y, MIN_ZOOM);
    HMM_Vec2 tl = camera_screen_to_world(&map->camera, HMM_V2(camera_b.X, camera_b.Y));
    HMM_Vec2 br = camera_screen_to_world(&map->camera, HMM_V2(camera_b.X + camera_b.W, camera_b.Y + camera_b.H));
    HMM_Vec2 tl_chunk = world_to_chunk(tl);
    HMM_Vec2 br_chunk = world_to_chunk(br);
    for (int y = (int)tl_chunk.Y; y <= (int)br_chunk.Y; y++)
        for (int x = (int)tl_chunk.X; x <= (int)br_chunk.X; x++)
            if (rect_intersect(chunk_bounds_ex(x, y), camera_b)) {
                struct chunk *chunk = map_chunk(map, x, y, true);
                assert(chunk != NULL);
            }
}

static void check_chunks(struct map *map) {
    map->delete_queue = NULL;
    map->delete_queue_size = 0;
    table_each(&map->chunks, map, ^(table_t *table, const char *key, table_entry_t *entry, void *userdata) {
        struct map *map = (struct map*)userdata;
        struct chunk *chunk = (struct chunk*)entry->value;
        assert(chunk != NULL);
        enum chunk_state last_state = chunk->state;
        bool changed = update_chunk_state(chunk, &map->camera);
        if (changed) {
            printf("CHUNK MODIFIED (%d, %d) (%s -> %s)\n", chunk->x, chunk->y, chunk_state_str(last_state), chunk_state_str(chunk->state));
            if (chunk->state == CHUNK_STATE_UNLOAD) {
                map->delete_queue = realloc(map->delete_queue, ++map->delete_queue_size * sizeof(uint64_t));
                map->delete_queue[map->delete_queue_size - 1] = chunk_index(chunk);
            }
        }
    });
}

static void release_chunks(struct map *map) {
    for (size_t i = 0; i < map->delete_queue_size; i++) {
        uint64_t index = map->delete_queue[i];
        struct chunk *chunk = NULL;
        if (table_get(&map->chunks, index, &chunk) && chunk != NULL) {
            printf("CHUNK REMOVED (%d, %d)\n", chunk->x, chunk->y);
            table_del(&map->chunks, index);
            chunk_destroy(chunk);
            free(chunk);
        }
    }
    free(map->delete_queue);
    map->delete_queue_size = 0;
}

void map_draw(struct map *map) {
    assert(map != NULL);
    check_chunks(map);
    release_chunks(map);
    find_chunks(map);
    draw_chunks(map);
}
