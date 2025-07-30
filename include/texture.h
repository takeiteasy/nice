//
//  texture.h
//  rpg
//
//  Created by George Watson on 24/07/2025.
//

#pragma once

#include "sokol/sokol_gfx.h"
#include "stb_image.h"
#include <assert.h>
#include <stdbool.h>

struct texture {
    int width, height;
    sg_image image;
    sg_sampler sampler;
};

bool texture_load_path(struct texture *texture, const char *path);
void texture_destroy(struct texture *texture);
