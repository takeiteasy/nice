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
#include "framebuffer.h"
#include "jobs.h"

struct map {
    table_t chunks;
    struct camera camera;
    struct texture tilemap;
    sg_shader shader;
    sg_pipeline pipeline;
    uint64_t *delete_queue;
    int delete_queue_size;
    struct job_pool worker;
};

bool map_create(struct map *map);
void map_destroy(struct map *map);
struct chunk* map_chunk(struct map *map, int x, int y, bool ensure);
void map_draw(struct map *map);
