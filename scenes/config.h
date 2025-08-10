//
//  config.h
//  rpg
//
//  Created by George Watson on 22/07/2025.
//

#define SCENES \
    X(test) \

#define FIRST_SCENE test

#define CHUNK_WIDTH  256
#define CHUNK_HEIGHT 256
#define CHUNK_SIZE ((CHUNK_WIDTH)*(CHUNK_HEIGHT))

#define CHUNK_FILL_CHANCE 40
#define CHUNK_SMOOTH_ITERATIONS 5
#define CHUNK_SURVIVE 4
#define CHUNK_STARVE 3

#define TILE_WIDTH 32
#define TILE_HEIGHT 32
#define TILE_ORIGINAL_WIDTH 8
#define TILE_ORIGINAL_HEIGHT 8
#define TILE_PADDING 4

#define MAX_ZOOM 2.f
#define MIN_ZOOM .2f