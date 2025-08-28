//
//  renderable.cpp
//  ice
//
//  Created by George Watson on 28/08/2025.
//

#include "renderable.hpp"
#include "global.hpp"
#include <vector>
#include <unordered_set>
#include <algorithm>

#define $Renderables Renderables::instance()

// Custom hash function for flecs::entity
namespace std {
    template<>
    struct hash<flecs::entity> {
        size_t operator()(const flecs::entity& entity) const {
            return hash<uint64_t>()(entity.id());
        }
    };
}

class Renderables: public Global<Renderables> {
private:
    std::unordered_set<flecs::entity> _entities;
    std::vector<flecs::entity> _sorted_entities;
    bool _needs_sort = false;
    flecs::world* _world = nullptr;

public:
    Renderables() = default;
    
    void set_world(flecs::world* world) {
        _world = world;
    }
    
    // Register a renderable entity
    void register_entity(flecs::entity entity) {
        if (_entities.insert(entity).second) {
            _sorted_entities.push_back(entity);
            _needs_sort = true;
        }
    }
    
    // Unregister a renderable entity
    void unregister_entity(flecs::entity entity) {
        if (_entities.erase(entity)) {
            auto it = std::find(_sorted_entities.begin(), _sorted_entities.end(), entity);
            if (it != _sorted_entities.end()) {
                _sorted_entities.erase(it);
            }
        }
    }
    
    // Get all renderable entities sorted by render layer
    const std::vector<flecs::entity>& get_sorted_entities() {
        if (_needs_sort && _world) {
            sort_entities();
            _needs_sort = false;
        }
        return _sorted_entities;
    }
    
    // Get count of registered entities
    size_t entity_count() const {
        return _entities.size();
    }
    
    // Check if entity is registered
    bool is_registered(flecs::entity entity) const {
        return _entities.find(entity) != _entities.end();
    }
    
    // Mark that entities need re-sorting (call when render layers change)
    void mark_dirty() {
        _needs_sort = true;
    }
    
    // Clear all registered entities
    void clear() {
        _entities.clear();
        _sorted_entities.clear();
        _needs_sort = false;
    }

private:
    void sort_entities() {
        if (!_world) return;
        
        std::sort(_sorted_entities.begin(), _sorted_entities.end(), 
                  [this](flecs::entity a, flecs::entity b) {
            const LuaRenderable* comp_a = a.get<LuaRenderable>();
            const LuaRenderable* comp_b = b.get<LuaRenderable>();
            
            if (!comp_a && !comp_b) return a.id() < b.id();
            if (!comp_a) return false;
            if (!comp_b) return true;
            
            return comp_a->render_layer < comp_b->render_layer;
        });
    }
};

// C++ API observer callbacks - correct signature
void OnRenderableAdd(flecs::entity entity, LuaRenderable& renderable) {
    $Renderables.register_entity(entity);
}

void OnRenderableRemove(flecs::entity entity, LuaRenderable& renderable) {
    $Renderables.unregister_entity(entity);
}

// Implementation of Renderable constructor
Renderable::Renderable(flecs::world &world) {
    world.module<Renderable>();

    // Register the LuaRenderable component with explicit name and meta information
    auto comp = world.component<LuaRenderable>("LuaRenderable")
        .member<bool>("visible")
        .member<int>("render_layer");

    // Set up observers to automatically manage renderable entities
    world.observer<LuaRenderable>()
        .event(flecs::OnAdd)
        .each(OnRenderableAdd);

    world.observer<LuaRenderable>()
        .event(flecs::OnRemove)
        .each(OnRenderableRemove);

    // Set the world reference for the Renderables manager
    $Renderables.set_world(&world);
}
