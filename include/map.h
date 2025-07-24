//
//  map.h
//  rpg
//
//  Created by George Watson on 23/07/2025.
//

#pragma once

#include "chunk.h"
#include "table.h"
#include "threads.h"
#include "rng.h"
#include "garry.h"
#include "bla.h"
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"

struct map {
    table_t chunks;
    struct thrd_pool pool;
    uint64_t *delete_queue;
    sg_pipeline pip;
    sg_pass_action pass_action;
    struct texture tilemap;
};

void map_create(struct map *map, const char *tilemap);
void map_destroy(struct map *map);

struct chunk* map_chunk(struct map *map, int x, int y);
