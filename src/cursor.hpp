//
//  cursor.hpp
//  rpg
//
//  Created by George Watson on 21/08/2025.
//

#pragma once

#include "global.hpp"
#include "entity.hpp"
#include "camera.hpp"

class Cursor: Entity<> {
    sg_shader _shader;
    sg_pipeline _pipeline;
    sg_bindings _bind;
    Texture *_texture;
    bool _dirty = true;

    void _build() {
        if (sg_query_buffer_state(_bind.vertex_buffers[0]) == SG_RESOURCESTATE_VALID)
            sg_destroy_buffer(_bind.vertex_buffers[0]);

        size_t size = sizeof(BasicVertex) * 6;
        sg_buffer_desc desc = {
            .usage.stream_update = true,
            .size = size,
        };
        _bind.vertex_buffers[0] = sg_make_buffer(&desc);

        glm::vec2 vsize = {CURSOR_WIDTH, CURSOR_HEIGHT};
        // Use the raw screen position directly, no world coordinate conversion
        BasicVertex *vertices = generate_quad(_position,
                                              vsize,
                                              {0, 0}, // offset
                                              {CURSOR_ORIGINAL_WIDTH, CURSOR_ORIGINAL_HEIGHT},
                                              {_texture->width(), _texture->height()});
        sg_range data = {
            .ptr = vertices,
            .size = size
        };
        sg_update_buffer(_bind.vertex_buffers[0], &data);
        _texture->bind(_bind);
    }

public:
    Cursor(): _texture($Assets.get<Texture>("hand.png")), Entity<>({0, 0}, 0, 0) {
        _shader = sg_make_shader(basic_shader_desc(sg_query_backend()));
        sg_pipeline_desc desc = {
            .shader = _shader,
            .layout = {
                .buffers[0].stride = sizeof(BasicVertex),
                .attrs = {
                    [ATTR_basic_position].format = SG_VERTEXFORMAT_FLOAT2,
                    [ATTR_basic_texcoord].format = SG_VERTEXFORMAT_FLOAT2
                }
            },
            .depth = {
                .pixel_format = SG_PIXELFORMAT_DEPTH,
                .compare = SG_COMPAREFUNC_LESS_EQUAL,
                .write_enabled = true
            },
            .cull_mode = SG_CULLMODE_BACK,
            .colors[0] = {
                .pixel_format = SG_PIXELFORMAT_RGBA8,
                .blend = {
                    .enabled = true,
                    .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                    .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .src_factor_alpha = SG_BLENDFACTOR_ONE,
                    .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
                }
            }
        };
        _pipeline = sg_make_pipeline(&desc);
        _position = glm::vec2(sapp_width(), sapp_height()) / 2.f;
    }

    ~Cursor() {
        if (sg_query_buffer_state(_bind.vertex_buffers[0]) == SG_RESOURCESTATE_VALID)
            sg_destroy_buffer(_bind.vertex_buffers[0]);
        if (sg_query_pipeline_state(_pipeline) == SG_RESOURCESTATE_VALID)
            sg_destroy_pipeline(_pipeline);
        if (sg_query_shader_state(_shader) == SG_RESOURCESTATE_VALID)
            sg_destroy_shader(_shader);
    }

    void set_position(glm::vec2 pos) {
        _position = pos;
        _dirty = true;
    }

    void draw() {
        if (_dirty)
            _build();
        sg_apply_pipeline(_pipeline);
        
        // Create a matrix that completely ignores camera transformations
        int w = framebuffer_width();
        int h = framebuffer_height();
        glm::mat4 projection = glm::ortho(0.f, (float)w, (float)h, 0.f, -1.f, 1.f);
        glm::mat4 view = glm::mat4(1.f); // Identity matrix - no camera transformations at all
        glm::mat4 mvp = projection * view;
        
        vs_params_t vs_params = { .mvp = mvp };
        sg_range params = SG_RANGE(vs_params);
        sg_apply_uniforms(UB_vs_params, &params);
        sg_apply_bindings(&_bind);
        sg_draw(0, 6, 1);
    }
};
