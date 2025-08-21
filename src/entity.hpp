//
//  game_object.hpp
//  rpg
//
//  Created by George Watson on 18/08/2025.
//

#pragma once

#include "vertex_batch.hpp"
#include "camera.hpp"
#include "texture.hpp"
#include "uuid.h"
#include "jobs.hpp"
#include <vector>
#include <shared_mutex>
#include <functional>
#include <unordered_map>
#include "basic.glsl.h"

struct BasicVertex {
    glm::vec2 position;
    glm::vec2 texcoord;
};

template<typename VT=BasicVertex>
class Entity {
    static_assert(std::is_base_of_v<BasicVertex, VT> == true || std::is_same_v<VT, BasicVertex> == true);
    uuid_v4 _uuid;

protected:
    glm::vec2 _position;

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
    Entity(glm::vec2 position, int chunk_x, int chunk_y): _uuid(uuid_v4::New()), _position(Camera::chunk_tile_to_world({chunk_x, chunk_y}, position)) {}
    Entity(const Entity &other): _uuid(uuid_v4::New()) {}
    Entity(Entity &&other) noexcept : _uuid(std::move(other._uuid)) {}

    // Assignment operators
    Entity &operator=(const Entity &other) {
        if (this != &other)
            _uuid = uuid_v4::New();
        return *this;
    }

    Entity &operator=(Entity &&other) noexcept {
        if (this != &other)
            _uuid = std::move(other._uuid);
        return *this;
    }

    bool operator==(const Entity &other) const {
        return std::memcmp(_uuid.data(), other._uuid.data(), 16 * sizeof(uint8_t)) == 0;
    }

    const uuid_v4 &uuid() const {
        return _uuid;
    }

    std::string uuid_as_string() {
        return _uuid.String();
    }

    virtual std::pair<VT*, size_t> vertices() {
        return {nullptr, 0};
    }
};

class ChunkManager;

template<typename T, typename VT=BasicVertex, int InitialCapacity=16, bool Dynamic=true>
class EntityFactory {
    static_assert(std::is_base_of_v<BasicVertex, VT> == true || std::is_same_v<VT, BasicVertex> == true);
    static_assert(std::is_base_of_v<Entity<VT>, T> == true);

protected:
    int _chunk_x, _chunk_y;
    ChunkManager *_chunks;
    Camera *_camera;
    Texture *_texture;
    std::vector<T*> _objects;
    mutable std::shared_mutex _objects_mutex;
    VertexBatch<VT, InitialCapacity, Dynamic> _batch;
    mutable std::shared_mutex _batch_mutex;
    std::atomic<bool> _dirty{false};

public:
    EntityFactory(ChunkManager *manager, Camera *camera, Texture *texture, int chunk_x, int chunk_y)
        : _chunks(manager)
        , _camera(camera)
        , _texture(texture)
        , _chunk_x(chunk_x)
        , _chunk_y(chunk_y) {
        _batch.set_texture(texture);
    }

    virtual void add_object(T *object) {
        std::lock_guard<std::shared_mutex> lock(_objects_mutex);
        _objects.push_back(object);
        _dirty.store(true);
    }

    virtual void add_objects(const std::vector<T*>& objects) {
        if (objects.empty())
            return;
        std::lock_guard<std::shared_mutex> lock(_objects_mutex);
        _objects.insert(_objects.end(), objects.begin(), objects.end());
        _dirty.store(true);
    }

    virtual bool is_dirty() const {
        return _dirty.load();
    }

    virtual void mark_dirty() {
        _dirty.store(true);
    }

    virtual bool build() {
        if (!_dirty.load())
            return false;
        std::lock_guard<std::shared_mutex> objects_lock(_objects_mutex);
        if (_objects.empty())
            return false;
        std::lock_guard<std::shared_mutex> lock(_batch_mutex);
        _batch.clear();
        _batch.reserve(_objects.size() * 6);
        for (const auto& object : _objects) {
            auto [vertices, count] = object->vertices();
            if (vertices != nullptr) {
                if (count > 0)
                    _batch.add_vertices(vertices, count);
                delete[] vertices;
            }
        }
        _batch.build();
        _dirty.store(false);
        return true;
    }

    virtual void draw() {
        std::shared_lock<std::shared_mutex> lock(_batch_mutex);
        if (_batch.empty() || !_batch.is_ready())
            return;
        vs_params_t vs_params = { .mvp = _camera->matrix() };
        sg_range params = SG_RANGE(vs_params);
        sg_apply_uniforms(UB_vs_params, &params);
        _batch.flush();
    }
};

template<typename T, typename FT>
class EntityFactoryManager {
    static_assert(std::is_base_of_v<EntityFactory<T>, FT> == true);
    using FactoryCreator = std::function<FT*(uint64_t)>;

    std::unordered_map<uint64_t, FT*> _factories;
    mutable std::shared_mutex _factories_mutex;
    FactoryCreator _factory_creator;
    ChunkManager *_chunks;

    JobPool _job_pool;
    ThreadSafeSet<uint64_t> _build_set;
    ThreadSafeSet<uint64_t> _remove_set;
    JobQueue<std::pair<uint64_t, FT*>> _build_queue;
    JobQueue<uint64_t> _remove_queue;

public:
    EntityFactoryManager(ChunkManager *manager, FactoryCreator factory_creator)
        : _chunks(manager)
        , _factory_creator(std::move(factory_creator))
        , _job_pool(std::thread::hardware_concurrency())
        , _build_queue([&](std::pair<uint64_t, FT*> factory_pair) {
            auto [id, factory] = factory_pair;
            if (factory->is_dirty())
                factory->build();
            _build_set.erase(id);
        })
        , _remove_queue([&](uint64_t id) {
            std::lock_guard<std::shared_mutex> lock(_factories_mutex);
            auto it = _factories.find(id);
            if (it != _factories.end()) {
                delete it->second;
                _factories.erase(it);
            }
            _remove_set.erase(id);
        }){}

    virtual  ~EntityFactoryManager() {
        clear();
    }

    virtual FT* get(uint64_t id, bool ensure = true) {
        std::lock_guard<std::shared_mutex> lock(_factories_mutex);
        auto it = _factories.find(id);
        if (it != _factories.end())
            return it->second;
        if (!ensure)
            return nullptr;
        auto factory = _factory_creator(id);
        _factories[id] = factory;
        return factory;
    }

    virtual void add_object(uint64_t id, T* object) {
        std::lock_guard<std::shared_mutex> lock(_factories_mutex);
        auto it = _factories.find(id);
        if (it != _factories.end())
            it->second->add_object(object);
    }

    virtual void add_objects(uint64_t id, const std::vector<T*>& objects) {
        std::lock_guard<std::shared_mutex> lock(_factories_mutex);
        auto it = _factories.find(id);
        if (it != _factories.end())
            it->second->add_objects(objects);
    }

    virtual void clear() {
        std::lock_guard<std::shared_mutex> lock(_factories_mutex);
        for (auto& [_, factory] : _factories)
            delete factory;
        _factories.clear();
    }
    
    virtual void build(uint64_t id) {
        std::shared_lock<std::shared_mutex> lock(_factories_mutex);
        auto it = _factories.find(id);
        if (it != _factories.end())
            if (it->second->is_dirty() && !_build_set.contains(id)) {
                _build_set.insert(id);
                _build_queue.push({id, it->second});
            }
    }

    virtual void remove(uint64_t id) {
        std::shared_lock<std::shared_mutex> lock(_factories_mutex);
        if (_factories.find(id) != _factories.end() && !_remove_set.contains(id)) {
            _remove_set.insert(id);
            _remove_queue.push(id);
        }
    }

    virtual void draw(uint64_t id) const {
        std::shared_lock<std::shared_mutex> lock(_factories_mutex);
        auto it = _factories.find(id);
        if (it != _factories.end())
            it->second->draw();
    }
};
