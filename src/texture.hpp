//
//  texture.hpp
//  nice
//
//  Created by George Watson on 03/08/2025.
//

#pragma once

#include "sokol/sokol_gfx.h"
#include "qoi.h"
#include "stb_image.h"
#include <string>
#include "asset_manager.hpp"

class Texture: public Asset<Texture> {
    int _width, _height;
    sg_image _image;
    sg_sampler _sampler;

public:
    int width() const {
        return _width;
    }

    int height() const {
        return _height;
    }

    void bind(sg_bindings& bindings) const {
        bindings.images[0] = _image;
        bindings.samplers[0] = _sampler;
    }

    bool load(const unsigned char *data, size_t data_size) override {
        unsigned char *pixels = NULL;
        if (strncmp((const char*)data, "qoif", 4) == 0) {
            qoi_desc desc;
            pixels = (unsigned char*)qoi_decode(data, (int)data_size, &desc, 4);
            _width = desc.width;
            _height = desc.height;
        } else {
            int w, h, c;
            pixels = stbi_load_from_memory(data, (int)data_size, &w, &h, &c, 4);
            _width = w;
            _height = h;
        }
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
        _image = sg_make_image(&idesc);
        sg_sampler_desc sdesc = {
            .min_filter = SG_FILTER_NEAREST,
            .mag_filter = SG_FILTER_NEAREST,
            .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
            .wrap_v = SG_WRAP_CLAMP_TO_EDGE
        };
        _sampler = sg_make_sampler(&sdesc);
        free(pixels);
        return sg_query_image_state(_image) == SG_RESOURCESTATE_VALID && sg_query_sampler_state(_sampler) == SG_RESOURCESTATE_VALID;
    }

    void unload() override {
        if (!sg_isvalid())
            return;
        if (sg_query_image_state(_image) == SG_RESOURCESTATE_VALID)
            sg_destroy_image(_image);
        if (sg_query_sampler_state(_sampler) == SG_RESOURCESTATE_VALID)
            sg_destroy_sampler(_sampler);
    }

    bool is_valid() const override {
        return sg_query_image_state(_image) == SG_RESOURCESTATE_VALID &&
               sg_query_sampler_state(_sampler) == SG_RESOURCESTATE_VALID;
    }

    std::string asset_extension() const override {
        return ".qoi";
    }

    operator sg_image() const {
        return _image;
    }

    operator sg_sampler() const {
        return _sampler;
    }
};
