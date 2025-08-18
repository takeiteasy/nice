//
//  game_object.hh
//  rpg
//
//  Created by George Watson on 18/08/2025.
//

#pragma once

#include "batch.hh"
#include "camera.hh"
#include "texture.hh"
#include "uuid.h"
#include <vector>
#include <shared_mutex>
#include "basic.glsl.h"

struct BasicVertex {
    glm::vec2 position;
    glm::vec2 texcoord;
};

template<typename VT=BasicVertex>
class GameObject {
    static_assert(std::is_base_of_v<BasicVertex, VT> == true || std::is_same_v<VT, BasicVertex> == true);
    uuid_t _uuid;

protected:
    glm::vec2 _position;
    int _texture_width, _texture_height;

    static BasicVertex* generate_quad(glm::vec2 position, glm::vec2 size, glm::vec2 clip_offset, glm::vec2 clip_size, glm::vec2 texture_size) {
        int _x = static_cast<int>(position.x);
        int _y = static_cast<int>(position.y);
        int _width = static_cast<int>(size.x);
        int _height = static_cast<int>(size.y);
        glm::vec2 _positions[] = {
            {_x, _y},                    // Top-left
            {_x + _width, _y},           // Top-right
            {_x + _width, _y + _height}, // Bottom-right
            {_x, _y + _height},          // Bottom-left
        };

        int _clip_x = static_cast<int>(clip_offset.x);
        int _clip_y = static_cast<int>(clip_offset.y);
        int _clip_width = static_cast<int>(clip_size.x);
        int _clip_height = static_cast<int>(clip_size.y);
        int _texture_width = static_cast<int>(texture_size.x);
        int _texture_height = static_cast<int>(texture_size.y);
        float iw = 1.f / _texture_width;
        float ih = 1.f / _texture_height;
        float tl = _clip_x * iw;
        float tt = _clip_y * ih;
        float tr = (_clip_x + _clip_width) * iw;
        float tb = (_clip_y + _clip_height) * ih;
        glm::vec2 _texcoords[4] = {
            {tl, tt}, // top left
            {tr, tt}, // top right
            {tr, tb}, // bottom right
            {tl, tb}, // bottom left
        };

        BasicVertex *v = new BasicVertex[6];
        static uint16_t _indices[] = {0, 1, 2, 2, 3, 0};
        for (int i = 0; i < 6; i++) {
            v[i].position = _positions[_indices[i]];
            v[i].texcoord = _texcoords[_indices[i]];
        }
        return v;
    }

public:
    GameObject(glm::vec2 position, int chunk_x, int chunk_y, int texture_width, int texture_height): _uuid(uuid_v4::New()), _position(Camera::chunk_tile_to_world({chunk_x, chunk_y}, position)), _texture_width(texture_width), _texture_height(texture_height) {}
    GameObject(const GameObject &other): _uuid(uuid_v4::New()) {}
    GameObject(GameObject &&other) noexcept : _uuid(std::move(other._uuid)) {}

    // Assignment operators
    GameObject &operator=(const GameObject &other) {
        if (this != &other)
            _uuid = uuid_v4::New();
        return *this;
    }

    GameObject &operator=(GameObject &&other) noexcept {
        if (this != &other)
            _uuid = std::move(other._uuid);
        return *this;
    }

    bool operator==(const GameObject &other) const {
        return std::memcmp(_uuid.data(), other._uuid.data(), 16 * sizeof(uint8_t)) == 0;
    }

    const uuid_t &uuid() const {
        return _uuid;
    }

    std::string uuid_as_string() {
        return _uuid.String();
    }

    virtual VT* vertices() const {
        return nullptr;
    }
};

template<typename T, typename VT=BasicVertex, int InitialCapacity=16, bool Dynamic=true>
class Factory {
    static_assert(std::is_base_of_v<BasicVertex, VT> == true || std::is_same_v<VT, BasicVertex> == true);
    static_assert(std::is_base_of_v<GameObject<VT>, T> == true);

protected:
    int _chunk_x, _chunk_y;
    Camera *_camera;
    Texture *_texture;
    std::vector<T*> _objects;
    mutable std::shared_mutex _objects_mutex;
    VertexBatch<VT, InitialCapacity, Dynamic> _batch;
    mutable std::shared_mutex _batch_mutex;
    std::atomic<bool> _dirty{false};

public:
    Factory(Camera *camera, Texture *texture, int chunk_x, int chunk_y): _camera(camera), _texture(texture), _chunk_x(chunk_x), _chunk_y(chunk_y) {
        _batch.set_texture(texture);
    }

    void add_object(T *object) {
        std::lock_guard<std::shared_mutex> lock(_objects_mutex);
        _objects.push_back(object);
        _dirty.store(true);
    }

    void add_objects(const std::vector<T*>& objects) {
        if (objects.empty())
            return;
        std::lock_guard<std::shared_mutex> lock(_objects_mutex);
        _objects.insert(_objects.end(), objects.begin(), objects.end());
        _dirty.store(true);
    }

    bool is_dirty() const {
        return _dirty.load();
    }

    void mark_dirty() {
        _dirty.store(true);
    }

    void build() {
        if (!_dirty.load())
            return;
        std::lock_guard<std::shared_mutex> lock(_batch_mutex);
        _batch.clear();
        _batch.reserve(_objects.size() * 6);
        for (const auto& object : _objects) {
            VT *vertices = object->vertices();
            _batch.add_vertices(vertices, 6);
            delete[] vertices;
        }
        _batch.build();
        _dirty.store(false);
    }

    void draw() {
        std::shared_lock<std::shared_mutex> lock(_batch_mutex);
        if (_batch.empty() || !_batch.is_ready())
            return;
        vs_params_t vs_params = { .mvp = _camera->matrix() };
        sg_range params = SG_RANGE(vs_params);
        sg_apply_uniforms(UB_vs_params, &params);
        _batch.flush();
    }
};
