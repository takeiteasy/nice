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
    pthread_rwlock_init(&map->chunks_lock, NULL);
    pthread_rwlock_init(&map->delete_queue_lock, NULL);

    return true;
}

void map_destroy(struct map *map) {
    if (!map)
        return;

    pool_destroy(&map->worker);
    sg_destroy_shader(map->shader);
    sg_destroy_pipeline(map->pipeline);
    texture_destroy(&map->tilemap);

    pthread_rwlock_wrlock(&map->delete_queue_lock);
    if (map->delete_queue)
        free(map->delete_queue);
    pthread_rwlock_unlock(&map->delete_queue_lock);
    pthread_rwlock_destroy(&map->delete_queue_lock);

    pthread_rwlock_wrlock(&map->chunks_lock);
    table_each(&map->chunks, NULL, ^(table_t *table, const char *key, table_entry_t *entry, void *userdata) {
        struct chunk *c = (struct chunk*)entry->value;
        // TODO: Export chunk to disk if needed
        chunk_destroy(c);
        free(c);
    });
    table_free(&map->chunks);
    pthread_rwlock_unlock(&map->chunks_lock);
    pthread_rwlock_destroy(&map->chunks_lock);
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

static void chunk_fill(struct chunk *c) {
    assert(c != NULL);

    uint8_t _grid[CHUNK_SIZE];
    cellular_automata(CHUNK_WIDTH, CHUNK_HEIGHT,
                      CHUNK_FILL_CHANCE, CHUNK_SMOOTH_ITERATIONS,
                      CHUNK_SURVIVE, CHUNK_STARVE,
                      _grid);
    pthread_rwlock_wrlock(&c->rwlock);
    for (int y = 0; y < CHUNK_HEIGHT; y++)
        for (int x = 0; x < CHUNK_WIDTH; x++)
            c->grid[y * CHUNK_WIDTH + x].solid = _grid[y * CHUNK_WIDTH + x];

    for (int y = 0; y < CHUNK_HEIGHT; y++)
        for (int x = 0; x < CHUNK_WIDTH; x++) {
            union tile *tile = &c->grid[y * CHUNK_WIDTH + x];
            tile->bitmask = tile->solid ? _bitmask(c, x, y, 0) : 0;
        }
    c->dirty = true;
    pthread_rwlock_unlock(&c->rwlock);
}

static struct chunk* add_chunk(struct map *map, int x, int y) {
    struct chunk *chunk = malloc(sizeof(struct chunk));
    if (!chunk_create(chunk, x, y, CHUNK_STATE_DORMANT)) {
        free(chunk);
        return NULL;
    }

    pthread_rwlock_wrlock(&map->chunks_lock);
    if (!table_set(&map->chunks, chunk_index(chunk), chunk)) {
        chunk_destroy(chunk);
        free(chunk);
        chunk = NULL;
    }
    pthread_rwlock_unlock(&map->chunks_lock);

    pool_add_job(&map->worker, map, ^(void *arg) {
        printf("FILLING CHUNK (%d, %d)\n", chunk->x, chunk->y);
        chunk_fill(chunk);
        printf("CHUNK FILLED (%d, %d)\n", chunk->x, chunk->y);
    });
    printf("CHUNK ADDED (%d, %d) (%s)\n", x, y, chunk_state_str(chunk->state));
    return chunk;
}

struct chunk* map_chunk(struct map *map, int x, int y, bool ensure) {
    assert(map != NULL);
    int index = chunk_index_ex(x, y);
    struct chunk *chunk = NULL;
    pthread_rwlock_rdlock(&map->chunks_lock);
    table_get(&map->chunks, index, &chunk);
    pthread_rwlock_unlock(&map->chunks_lock);
    if (chunk != NULL)
        return chunk;
    if (!ensure)
        return NULL;
    // TODO: Attempt to find chunk on disk
    return add_chunk(map, x, y);
}

static bool update_chunk_state(struct chunk *chunk, struct camera *camera) {
    pthread_rwlock_wrlock(&chunk->rwlock);
    struct rect chunk_b = chunk_bounds(chunk);
    enum chunk_state state = chunk->state;
    if (!rect_intersect(camera_bounds_ex(camera->position.X, camera->position.Y, MIN_ZOOM), chunk_b))
        chunk->state = CHUNK_STATE_UNLOAD;
    else
        chunk->state = rect_intersect(camera_bounds(camera), chunk_b) ? CHUNK_STATE_ACTIVE : CHUNK_STATE_DORMANT;
    pthread_rwlock_unlock(&chunk->rwlock);
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
        if (!update_chunk_state(chunk, &map->camera))
            return;
        printf("CHUNK MODIFIED (%d, %d) (%s -> %s)\n", chunk->x, chunk->y, chunk_state_str(last_state), chunk_state_str(chunk->state));
        if (chunk->state == CHUNK_STATE_UNLOAD) {
            pthread_rwlock_wrlock(&map->delete_queue_lock);
            map->delete_queue = realloc(map->delete_queue, ++map->delete_queue_size * sizeof(uint64_t));
            map->delete_queue[map->delete_queue_size - 1] = chunk_index(chunk);
            pthread_rwlock_unlock(&map->delete_queue_lock);
        }
    });
}

static void release_chunks(struct map *map) {
    pthread_rwlock_wrlock(&map->delete_queue_lock);
    for (size_t i = 0; i < map->delete_queue_size; i++) {
        uint64_t index = map->delete_queue[i];
        struct chunk *chunk = NULL;
        pthread_rwlock_wrlock(&map->chunks_lock);
        if (table_get(&map->chunks, index, &chunk) && chunk != NULL) {
            printf("CHUNK REMOVED (%d, %d)\n", chunk->x, chunk->y);
            table_del(&map->chunks, index);
            chunk_destroy(chunk);
            free(chunk);
        }
        pthread_rwlock_unlock(&map->chunks_lock);
    }
    free(map->delete_queue);
    map->delete_queue_size = 0;
    pthread_rwlock_unlock(&map->delete_queue_lock);
}

static void draw_chunks(struct map *map) {
    sg_apply_pipeline(map->pipeline);
    table_each(&map->chunks, NULL, ^(table_t *table, const char *key, table_entry_t *entry, void *userdata) {
        struct chunk *chunk = (struct chunk*)entry->value;
        chunk_draw(chunk, &map->tilemap, &map->camera);
    });
}

void map_draw(struct map *map) {
    assert(map != NULL);
    check_chunks(map);
    release_chunks(map);
    find_chunks(map);
    draw_chunks(map);
}
