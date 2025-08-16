//
//  texture.hh
//  rpg
//
//  Created by George Watson on 03/08/2025.
//

#pragma once

#include "assets.hh"
#include "sokol/sokol_gfx.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <string>

class Texture: public Asset<Texture> {
    int _width, _height;
    sg_image image;
    sg_sampler sampler;

public:
    int width() const {
        return _width;
    }

    int height() const {
        return _height;
    }

    void bind(sg_bindings& bindings) const {
        bindings.images[0] = image;
        bindings.samplers[0] = sampler;
    }

    bool load(const std::string& path) override {
        int num_channels;
        const int desired_channels = 4;
        stbi_uc* pixels = stbi_load(path.c_str(), &_width, &_height, &num_channels, desired_channels);
        if (!pixels)
            return false;
        sg_image_desc idesc = {
            .width = _width,
            .height = _height,
            .pixel_format = SG_PIXELFORMAT_RGBA8,
            .data.subimage[0][0] = {
                .ptr = pixels,
                .size = (size_t)(_width * _height * 4),
            }
        };
        image = sg_make_image(&idesc);
        sg_sampler_desc sdesc = {
            .min_filter = SG_FILTER_NEAREST,
            .mag_filter = SG_FILTER_NEAREST,
            .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
            .wrap_v = SG_WRAP_CLAMP_TO_EDGE
        };
        sampler = sg_make_sampler(&sdesc);
        stbi_image_free(pixels);
        return sg_query_image_state(image) == SG_RESOURCESTATE_VALID &&
        sg_query_sampler_state(sampler) == SG_RESOURCESTATE_VALID;
    }

    void unload() override {
        if (!sg_isvalid())
            return;
        if (sg_query_image_state(image) == SG_RESOURCESTATE_VALID)
            sg_destroy_image(image);
        if (sg_query_sampler_state(sampler) == SG_RESOURCESTATE_VALID)
            sg_destroy_sampler(sampler);
    }

    bool is_valid() const override {
        return sg_query_image_state(image) == SG_RESOURCESTATE_VALID &&
               sg_query_sampler_state(sampler) == SG_RESOURCESTATE_VALID;
    }
};
