//
//  Untitled.h
//  rpg
//
//  Created by George Watson on 24/07/2025.
//

#pragma once

#include "generic_image.h"
#include "sokol/sokol_gfx.h"

struct texture {
    unsigned int width, height;
    sg_image image;
    sg_sampler sampler;
};

struct texture texture_create(unsigned int width, unsigned int height);
void texture_destroy(struct texture *texture);
bool texture_load_path(struct texture *texture, const char *path);
bool texture_load_memory(struct texture *texture, unsigned char *data, size_t data_size);
void texture_update(struct texture *texture, image_t img);
void texture_set_sampler(struct texture *texture, sg_filter min_filter, sg_filter mag_filter, sg_wrap wrap_u, sg_wrap wrap_v);
