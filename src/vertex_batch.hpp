//
//  vertex_batch.hpp
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
#include "texture.hpp"
#include "glm/vec2.hpp"
#include "glm/vec4.hpp"

template <typename T, int InitialCapacity=16, bool Dynamic=true>
class VertexBatch {
    static_assert(InitialCapacity > 0, "InitialCapacity must be greater than 0");

    Texture *_texture = nullptr;
    sg_bindings _bind = {SG_INVALID_ID};
    size_t _capacity;
    size_t _count = 0;
    std::unique_ptr<T[]> _vertices;

    void resize(size_t new_capacity) {
        auto new_vertices = std::make_unique<T[]>(new_capacity);
        std::copy(_vertices.get(), _vertices.get() + _count, new_vertices.get());
        _vertices = std::move(new_vertices);
        _capacity = new_capacity;
    }

public:
    VertexBatch(Texture *texture=nullptr): _texture(texture) {
        _capacity = InitialCapacity;
        _vertices = std::make_unique<T[]>(_capacity);
    }

    VertexBatch(const VertexBatch&) = delete;
    VertexBatch& operator=(const VertexBatch&) = delete;

    VertexBatch(VertexBatch&& other) noexcept 
        : _bind(other._bind)
        , _capacity(other._capacity)
        , _count(other._count)
        , _vertices(std::move(other._vertices))
    {
        other._bind = {};
        other._capacity = 0;
        other._count = 0;
    }

    VertexBatch& operator=(VertexBatch&& other) noexcept {
        if (this != &other) {
            if (sg_query_buffer_state(_bind.vertex_buffers[0]) == SG_RESOURCESTATE_VALID)
                sg_destroy_buffer(_bind.vertex_buffers[0]);

            _bind = other._bind;
            _capacity = other._capacity;
            _count = other._count;
            _vertices = std::move(other._vertices);

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

    void set_texture(Texture* texture) {
        _texture = texture;
    }

    void add_vertices(const T* vertices, size_t count) {
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

    void reserve(size_t new_capacity) {
        if (new_capacity > _capacity)
            resize(new_capacity);
    }

    void clear() {
        _count = 0;
        if (Dynamic) {
            _capacity = InitialCapacity;
            _vertices = std::make_unique<T[]>(_capacity);
        } else
            std::memset(_vertices.get(), 0, sizeof(T) * _capacity);
    }

    bool is_ready() const {
        return sg_query_buffer_state(_bind.vertex_buffers[0]) == SG_RESOURCESTATE_VALID;
    }

    bool build() {
        if (_count == 0)
            return false;

        size_t required_size = sizeof(T) * _count;
        size_t required_buffer_size = sizeof(T) * _capacity;
        
        // Always recreate the buffer to ensure it's the right size
        // This is safer and handles dynamic resizing properly
        if (sg_query_buffer_state(_bind.vertex_buffers[0]) == SG_RESOURCESTATE_VALID) {
            sg_destroy_buffer(_bind.vertex_buffers[0]);
        }
        
        sg_buffer_desc desc = {
            .usage.stream_update = true,
            .size = required_buffer_size
        };
        _bind.vertex_buffers[0] = sg_make_buffer(&desc);
        
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
        if (!is_ready())
            throw std::runtime_error("VertexBatch is not built");
        sg_apply_bindings(&_bind);
        sg_draw(0, (int)_count, 1);
        if (empty_after)
            clear();
    }
    
    size_t count() const { return _count; }
    size_t capacity() const { return _capacity; }
    bool empty() const { return _count == 0; }
    bool full() const { return Dynamic ? false : _count >= _capacity; }
};
