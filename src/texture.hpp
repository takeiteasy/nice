//
//  texture.hpp
//  nice
//
//  Created by George Watson on 03/08/2025.
//

#pragma once

#include "sokol/sokol_gfx.h"
#include "qoi.h"
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

    bool load(const unsigned char *data, size_t data_size) override {
        qoi_desc desc;
        unsigned char *pixels = (unsigned char*)qoi_decode(data, (int)data_size, &desc, 4);
        if (!pixels)
            return false;
        _width = desc.width;
        _height = desc.height;
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
        free(pixels);
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

    std::string asset_extension() const override {
        return ".qoi";
    }
};
