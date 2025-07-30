//
//  map.h
//  rpg
//
//  Created by George Watson on 30/07/2025.
//

#pragma once

#include "chunk.h"
#include "camera.h"
#include "texture.h"
#include "table.h"
#include "threads.h"

struct map {
    table_t chunks;
    struct camera camera;
    struct texture tilemap;
    sg_shader shader;
    sg_pipeline pipeline;
    struct thrd_pool pool;
    struct thrd_queue delete_queue;
};

bool map_create(struct map *map);
void map_destroy(struct map *map);
struct chunk* map_chunk(struct map *map, int x, int y, bool ensure);
void map_draw(struct map *map);
void map_update(struct map *map);
