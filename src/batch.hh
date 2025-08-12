//
//  batch.hh
//  rpg
//
//  Created by George Watson on 11/08/2025.
//

#pragma once

#include "sokol/sokol_gfx.h"
#include <assert.h>
#include <stdexcept>
#include <algorithm>
#include <memory>
#include "texture.hh"
#include "glm/vec2.hpp"
#include "glm/vec4.hpp"

struct SpriteVertex {
    glm::vec2 position;
    glm::vec2 texcoord;
    glm::vec4 color;
};

template <typename T=SpriteVertex, int InitialCapacity=16, bool Dynamic=true>
class VertexBatch {
    static_assert(InitialCapacity > 0, "InitialCapacity must be greater than 0");

    Texture *_texture = nullptr;
    sg_pipeline _pipeline = {SG_INVALID_ID};
    sg_bindings _bind = {SG_INVALID_ID};
    int _capacity;
    int _count = 0;
    std::unique_ptr<T[]> _vertices;

    void resize(int new_capacity) {
        auto new_vertices = std::make_unique<T[]>(new_capacity);
        std::copy(_vertices.get(), _vertices.get() + _count, new_vertices.get());
        _vertices = std::move(new_vertices);
        _capacity = new_capacity;
    }

public:
    VertexBatch(const sg_pipeline& pipeline={SG_INVALID_ID}, Texture *texture=nullptr): _pipeline(pipeline), _texture(texture) {
        _capacity = InitialCapacity;
        _vertices = std::make_unique<T[]>(_capacity);
    }

    VertexBatch(const VertexBatch&) = delete;
    VertexBatch& operator=(const VertexBatch&) = delete;

    VertexBatch(VertexBatch&& other) noexcept 
        : _pipeline(other._pipeline)
        , _bind(other._bind)
        , _capacity(other._capacity)
        , _count(other._count)
        , _vertices(std::move(other._vertices))
    {
        other._pipeline = {SG_INVALID_ID};
        other._bind = {};
        other._capacity = 0;
        other._count = 0;
    }

    VertexBatch& operator=(VertexBatch&& other) noexcept {
        if (this != &other) {
            if (sg_query_buffer_state(_bind.vertex_buffers[0]) == SG_RESOURCESTATE_VALID)
                sg_destroy_buffer(_bind.vertex_buffers[0]);
            if (sg_query_pipeline_state(_pipeline) == SG_RESOURCESTATE_VALID)
                sg_destroy_pipeline(_pipeline);

            _pipeline = other._pipeline;
            _bind = other._bind;
            _capacity = other._capacity;
            _count = other._count;
            _vertices = std::move(other._vertices);

            other._pipeline = {SG_INVALID_ID};
            other._bind = {};
            other._capacity = 0;
            other._count = 0;
        }
        return *this;
    }

    ~VertexBatch() {
        if (sg_query_buffer_state(_bind.vertex_buffers[0]) == SG_RESOURCESTATE_VALID)
            sg_destroy_buffer(_bind.vertex_buffers[0]);
    }

    void set_pipeline(const sg_pipeline& pipeline) {
        _pipeline = pipeline;
    }

    void set_pipeline(const sg_pipeline_desc& desc) {
        if (sg_query_pipeline_state(_pipeline) == SG_RESOURCESTATE_VALID)
            sg_destroy_pipeline(_pipeline);
        _pipeline = sg_make_pipeline(&desc);
        assert(sg_query_pipeline_state(_pipeline) == SG_RESOURCESTATE_VALID);
    }

    void set_texture(Texture* texture) {
        _texture = texture;
    }

    void add(const T& vertex) {
        if (_count >= _capacity) {
            if (Dynamic)
                resize(_capacity * 2);
            else
                throw std::runtime_error("VertexBatch would exceed fixed size");
        }
        _vertices[_count++] = vertex;
    }

public:
    void add_vertices(const T* vertices, int count) {
        if (_count + count > _capacity) {
            if (Dynamic)
                while (_count + count > _capacity)
                    resize(_capacity * 2);
            else
                throw std::runtime_error("VertexBatch would exceed fixed size");
        }
        std::copy(vertices, vertices + count, _vertices.get() + _count);
        _count += count;
    }

    void reserve(int new_capacity) {
        if (new_capacity > _capacity)
            resize(new_capacity);
    }

    int count() const { return _count; }
    int capacity() const { return _capacity; }
    bool empty() const { return _count == 0; }
    bool full() const { return Dynamic ? false : _count >= _capacity; }
    void clear() { _count = 0; }
    const T* data() const { return _vertices.get(); }
    T* data() { return _vertices.get(); }

    bool is_valid() const {
        return sg_query_buffer_state(_bind.vertex_buffers[0]) == SG_RESOURCESTATE_VALID;
    }

    bool build() {
        if (_count == 0)
            return false;

        size_t required_size = sizeof(T) * _count;
        if (sg_query_buffer_state(_bind.vertex_buffers[0]) != SG_RESOURCESTATE_VALID) {
            sg_buffer_desc desc = {
                .usage.stream_update = true,
                .size = sizeof(T) * _capacity
            };
            _bind.vertex_buffers[0] = sg_make_buffer(&desc);
        }
        sg_range data = {
            .ptr = _vertices.get(),
            .size = required_size
        };
        sg_update_buffer(_bind.vertex_buffers[0], &data);
        if (_texture != nullptr)
            _texture->bind(_bind);

        return true;
    }

    void flush(bool empty_after=false) {
        if (!is_valid())
            throw std::runtime_error("VertexBatch is not built");
        if (sg_query_pipeline_state(_pipeline) == SG_RESOURCESTATE_VALID)
            sg_apply_pipeline(_pipeline);
        sg_apply_bindings(&_bind);
        sg_draw(0, _count, 1);
        if (empty_after)
            _count = 0;
    }
};
