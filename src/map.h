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
#include "garry.h"
#include "bla.h"
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"

struct map {
    table_t chunks;
    struct thrd_pool *pool;
    uint64_t *delete_queue;
};
