//
//  map.h
//  rpg
//
//  Created by George Watson on 30/07/2025.
//

#pragma once

#include "table.h"
#include "chunk.h"
#include "camera.h"
#include "texture.h"
#include "jobs.h"

struct map {
    table_t chunks;
    pthread_rwlock_t chunks_lock;
    struct camera camera;
    struct texture tilemap;
    sg_shader shader;
    sg_pipeline pipeline;
    uint64_t *delete_queue;
    int delete_queue_size;
    pthread_rwlock_t delete_queue_lock;
    struct job_pool worker;
};

bool map_create(struct map *map);
void map_destroy(struct map *map);
struct chunk* map_chunk(struct map *map, int x, int y, bool ensure);
void map_draw(struct map *map);
