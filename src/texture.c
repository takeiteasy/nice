//
//  texture.c
//  rpg
//
//  Created by George Watson on 24/07/2025.
//

#define GENERIC_IMAGE_IMPLEMENTATION
#include "texture.h"

struct texture texture_create(unsigned int width, unsigned int height) {
    return (struct texture) {
        .width = width,
        .height = height,
        .image = sg_make_image(&(sg_image_desc) {
            .width = width,
            .height = height,
            .pixel_format = SG_PIXELFORMAT_RGBA8,
            .usage.stream_update = true
        }),
        .sampler = sg_make_sampler(&(sg_sampler_desc) {
            .min_filter = SG_FILTER_NEAREST,
            .mag_filter = SG_FILTER_NEAREST,
            .wrap_u = SG_WRAP_CLAMP_TO_BORDER,
            .wrap_v = SG_WRAP_CLAMP_TO_BORDER
        })
    };
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

struct texture texture_load_path(const char *path) {
    image_t tmp = image_load_from_path(path);
    if (!tmp)
        return (struct texture){0};
    struct texture result = texture_create(image_width(tmp), image_height(tmp));
    if (!result.image.id || !result.sampler.id)
        return (struct texture){0};
    texture_update(&result, tmp);
    return result;
}

struct texture texture_load_memory(unsigned char *data, size_t data_size) {
    image_t tmp = image_load_from_memory(data, data_size);
    if (!tmp)
        return (struct texture){0};
    struct texture result = texture_create(image_width(tmp), image_height(tmp));
    if (!result.image.id || !result.sampler.id)
        return (struct texture){0};
    texture_update(&result, tmp);
    return result;
}

void texture_update(struct texture *texture, image_t img) {
    if (texture->image.id == SG_INVALID_ID)
        return;
    sg_update_image(texture->image, &(sg_image_data) {
        .subimage[0][0] = (sg_range) {
            .ptr = img,
            .size = image_width(img) * image_height(img) * sizeof(int)
        }
    });
}

void texture_set_sampler(struct texture *texture, sg_filter min_filter, sg_filter mag_filter, sg_wrap wrap_u, sg_wrap wrap_v) {
    if (texture->sampler.id != SG_INVALID_ID)
        sg_destroy_sampler(texture->sampler);
    texture->sampler = sg_make_sampler(&(sg_sampler_desc) {
        .min_filter = min_filter,
        .mag_filter = mag_filter,
        .wrap_u = wrap_u,
        .wrap_v = wrap_v
    });
}
