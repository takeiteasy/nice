//
//  texture.c
//  rpg
//
//  Created by George Watson on 24/07/2025.
//

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "texture.h"
#include <assert.h>

bool texture_load_path(struct texture *texture, const char *path) {
    assert(texture != NULL && path != NULL);
    int num_channels;
    const int desired_channels = 4;
    stbi_uc* pixels = stbi_load(path, &texture->width, &texture->height, &num_channels, desired_channels);
    if (!pixels)
        return false;
    texture->image = sg_make_image(&(sg_image_desc){
        .width = texture->width,
        .height = texture->height,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data.subimage[0][0] = {
            .ptr = pixels,
            .size = (size_t)(texture->width * texture->height * 4),
        }
    });
    texture->sampler = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE
    });
    stbi_image_free(pixels);
    return sg_query_image_state(texture->image) == SG_RESOURCESTATE_VALID;
}

void texture_destroy(struct texture *texture) {
    if (!texture)
        return;
    if (texture->image.id != SG_INVALID_ID)
        sg_destroy_image(texture->image);
    if (texture->sampler.id != SG_INVALID_ID)
        sg_destroy_sampler(texture->sampler);
    *texture = (struct texture){0};
}
