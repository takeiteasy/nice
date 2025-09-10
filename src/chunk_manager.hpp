//
//  chunk_manager.hpp
//  nice
//
//  Created by George Watson on 07/09/2025.
//

#pragma once

#include "global.hpp"
#include "chunk.hpp"
#include "job_queue.hpp"
#include "camera.hpp"
#include "fmt/format.h"
#include <unordered_map>
#include <shared_mutex>
#include <iostream>
#include <filesystem>
#include <queue>
#include <mutex>
#include "uuid.h"
#include "sokol/sokol_time.h"
#include "minilua.h"

#define $Chunks ChunkManager::instance()

struct ChunkEvent {
    enum Type {
        Created,
        Deleted,
        VisibilityChanged
    } type;
    int x, y;
    ChunkVisibility old_vis = ChunkVisibility::OutOfSign;
    ChunkVisibility new_vis = ChunkVisibility::OutOfSign;
};

class ChunkManager: public Global<ChunkManager> {
    std::unordered_map<uint64_t, Chunk*> _chunks;
    mutable std::shared_mutex _chunks_lock;
    JobQueue<std::pair<int, int>>* _create_chunk_queue;
    ThreadSafeSet<uint64_t> _chunks_being_created;
    JobQueue<Chunk*>* _build_chunk_queue;
    ThreadSafeSet<uint64_t> _chunks_being_built;
    ThreadSafeSet<uint64_t> _chunks_being_destroyed;
    std::unordered_map<uint64_t, uint64_t> _deletion_queue;
    mutable std::shared_mutex _deletion_queue_lock;
    
    Camera *_camera = nullptr;
    Texture *_tilemap = nullptr;
    uuid::v4::UUID _world_id;
    
    // Lua event handling
    lua_State *_lua_state = nullptr;
    std::unordered_map<int, int> _chunk_callbacks;
    std::queue<ChunkEvent> _chunk_event_queue;
    std::mutex _event_queue_mutex;
    
    std::string _get_world_directory() {
        std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
        std::filesystem::path world_dir = temp_dir / _world_id.String();
        if (!std::filesystem::exists(world_dir))
            std::filesystem::create_directories(world_dir);
        return world_dir.string();
    }

    std::string _get_chunk_filepath(int x, int y) {
        uint64_t chunk_index = index(x, y);
        std::string filename = std::to_string(chunk_index) + ".nicechunk";
        std::filesystem::path world_dir = _get_world_directory();
        return (world_dir / filename).string();
    }
    
    void call_lua_chunk_event(ChunkEvent::Type event_type, int x, int y, ChunkVisibility old_vis = ChunkVisibility::OutOfSign, ChunkVisibility new_vis = ChunkVisibility::OutOfSign) {
        if (!_lua_state) return;
        
        auto it = _chunk_callbacks.find(static_cast<int>(event_type));
        if (it == _chunk_callbacks.end())
            return;
        lua_rawgeti(_lua_state, LUA_REGISTRYINDEX, it->second); // Get the callback function
        if (lua_isfunction(_lua_state, -1)) {
            lua_pushinteger(_lua_state, x);
            lua_pushinteger(_lua_state, y);
            if (event_type == ChunkEvent::VisibilityChanged) {
                lua_pushinteger(_lua_state, static_cast<int>(old_vis));
                lua_pushinteger(_lua_state, static_cast<int>(new_vis));
                lua_pcall(_lua_state, 4, 0, 0);
            } else {
                lua_pcall(_lua_state, 2, 0, 0);
            }
        } else
            lua_pop(_lua_state, 1); // Remove non-function from stack
    }
    
public:
    ~ChunkManager() {
        clear();
        delete _create_chunk_queue;
        delete _build_chunk_queue;
    }

    bool is_empty() const {
        return _build_chunk_queue->empty() && _create_chunk_queue->empty() &&
               _chunks_being_built.empty() && _chunks_being_created.empty();
    }

    void initialize(Camera *camera, Texture *tilemap, uuid::v4::UUID world_id) {
        _camera = camera;
        _tilemap = tilemap;
        _world_id = world_id;
        
        // Initialize the job queues with their callbacks
        _create_chunk_queue = new JobQueue<std::pair<int, int>>([this](std::pair<int, int> coords) {
            auto [x, y] = coords;
            uint64_t idx = index(x, y);
            Chunk *chunk = new Chunk(x, y, _camera, _tilemap);
            
            // Try to load from disk first
            std::string chunk_filepath = _get_chunk_filepath(x, y);
            bool loaded_from_disk = false;
            if (std::filesystem::exists(chunk_filepath))
                try {
                    chunk->deserialize(chunk_filepath.c_str());
                    loaded_from_disk = true;
                    std::cout << fmt::format("Loaded chunk at ({}, {}) from {}\n", x, y, chunk_filepath);
                } catch (const std::exception& e) {
                    std::cout << fmt::format("Error loading chunk at ({}, {}) from {}: {}\n", x, y, chunk_filepath, e.what());
                }
            
            {
                std::unique_lock<std::shared_mutex> lock(_chunks_lock);
                std::cout << fmt::format("New chunk created at ({}, {})\n", x, y);
                _chunks[idx] = chunk;
                _chunks_being_created.erase(idx);  // Remove from being created set
                _chunks_being_built.insert(idx);  // Mark as being built
            }
            
            // Queue chunk created event
            {
                std::lock_guard<std::mutex> lock(_event_queue_mutex);
                _chunk_event_queue.push({ChunkEvent::Created, x, y});
            }
            
            // Only fill if we didn't load from disk
            if (!loaded_from_disk) {
                chunk->fill();
                std::cout << fmt::format("Chunk at ({}, {}) finished filling\n", x, y);
                try {
                    chunk->serialize(chunk_filepath.c_str());
                } catch (const std::exception& e) {
                    std::cout << fmt::format("Error saving chunk at ({}, {}) to {}: {}\n", x, y, chunk_filepath, e.what());
                }
            }

            _build_chunk_queue->enqueue(chunk);
        });
        
        _build_chunk_queue = new JobQueue<Chunk*>([this](Chunk *chunk) {
            chunk->build();
            std::cout << fmt::format("Chunk at ({}, {}) finished building\n", chunk->x(), chunk->y());

            // Remove from being built set after successful build
            uint64_t idx = chunk->id();
            _chunks_being_built.erase(idx);
        });
    }
    
    void set_lua_state(lua_State *L) {
        _lua_state = L;
    }
    
    void get_chunk(int x, int y, std::function<void(Chunk*)> callback) {
        Chunk* chunk = nullptr;
        uint64_t idx = index(x, y);
        {
            std::shared_lock<std::shared_mutex> lock(_chunks_lock);
            auto it = _chunks.find(idx);
            if (it != _chunks.end())
                if ((chunk = it->second) != nullptr && chunk->is_filled())
                    callback(it->second);
        }
    }
    
    void ensure_chunk(int x, int y, bool priority) {
        uint64_t idx = index(x, y);

        // Check and insert atomically using ThreadSafeSet methods first
        if (_chunks_being_created.contains(idx) ||
            _chunks_being_built.contains(idx))
            return;

        // Try to mark as being created
        if (!_chunks_being_created.insert(idx))
            return; // Another thread already marked it

        // Check if chunk already exists (after marking as being created to avoid race)
        bool chunk_exists = false;
        {
            std::shared_lock<std::shared_mutex> chunks_lock(_chunks_lock);
            chunk_exists = (_chunks.find(idx) != _chunks.end());
        }

        if (chunk_exists) {
            // Chunk exists, so we don't need to create it
            _chunks_being_created.erase(idx);
            // Remove from deletion queue if it was there
            {
                std::unique_lock<std::shared_mutex> deletion_lock(_deletion_queue_lock);
                _deletion_queue.erase(idx);
            }
            return;
        }

        // Remove from deletion queue if it was there
        {
            std::unique_lock<std::shared_mutex> deletion_lock(_deletion_queue_lock);
            _deletion_queue.erase(idx);
        }

        if (priority)
            _create_chunk_queue->enqueue_priority({x, y});
        else
            _create_chunk_queue->enqueue({x, y});
    }
    
    void update_chunks(const Rect &camera_bounds, const Rect &max_bounds) {
        // Collect chunks to update without holding the lock
        std::vector<Chunk*> chunks_to_update;
        {
            std::shared_lock<std::shared_mutex> lock(_chunks_lock);
            chunks_to_update.reserve(_chunks.size());
            for (const auto& [id, chunk] : _chunks)
                chunks_to_update.push_back(chunk);
        }

        // Collect deletion updates and events
        std::vector<std::pair<uint64_t, bool>> deletion_updates;
        std::vector<ChunkEvent> events;
        deletion_updates.reserve(chunks_to_update.size());
        events.reserve(chunks_to_update.size());

        // Update chunks without holding any locks
        for (auto chunk : chunks_to_update) {
            if (!chunk->is_ready())
                continue;

            ChunkVisibility last_visibility = chunk->visibility();
            Rect chunk_bounds = chunk->bounds();
            ChunkVisibility new_visibility = !max_bounds.intersects(chunk_bounds) ? ChunkVisibility::OutOfSign :
                                             camera_bounds.intersects(chunk_bounds) ? ChunkVisibility::Visible :
                                             ChunkVisibility::Occluded;
            chunk->set_visibility(new_visibility);
            if (new_visibility != last_visibility) {
                std::cout << fmt::format("Chunk at ({}, {}) visibility changed from {} to {}\n",
                                         chunk->x(), chunk->y(),
                                         Chunk::visibility_to_string(last_visibility),
                                         Chunk::visibility_to_string(new_visibility));
                
                // Collect deletion queue updates to apply later
                uint64_t chunk_id = chunk->id();
                switch (new_visibility) {
                    case ChunkVisibility::Visible:
                    case ChunkVisibility::Occluded:
                        // Mark for removal from deletion queue
                        deletion_updates.emplace_back(chunk_id, false);
                        break;
                    case ChunkVisibility::OutOfSign:
                        // Mark for addition to deletion queue
                        deletion_updates.emplace_back(chunk_id, true);
                        break;
                }
                
                // Collect events to process later
                events.push_back({ChunkEvent::VisibilityChanged, chunk->x(), chunk->y(), last_visibility, new_visibility});
            }
        }

        // Apply deletion queue updates in batch
        if (!deletion_updates.empty()) {
            uint64_t now = stm_now();
            std::unique_lock<std::shared_mutex> lock(_deletion_queue_lock);
            for (const auto& [chunk_id, should_add] : deletion_updates) {
                if (should_add)
                    _deletion_queue[chunk_id] = now;
                else
                    _deletion_queue.erase(chunk_id);
            }
        }
        
        // Apply events in batch
        if (!events.empty()) {
            std::lock_guard<std::mutex> lock(_event_queue_mutex);
            for (const auto& event : events)
                _chunk_event_queue.push(event);
        }
    }

    void update_deletion_queue() {
        auto now = stm_now();
        std::vector<uint64_t> chunks_to_destroy;
        {
            std::unique_lock<std::shared_mutex> lock(_deletion_queue_lock);
            for (auto it = _deletion_queue.begin(); it != _deletion_queue.end();) {
                if (stm_sec(stm_diff(now, it->second)) > CHUNK_DELETION_TIMEOUT) {
                    std::cout << fmt::format("Chunk with ID {} exceeded deletion timeout, marking for destruction\n", it->first);
                    chunks_to_destroy.push_back(it->first);
                    it = _deletion_queue.erase(it);
                } else
                    ++it;
            }
        }
        
        // Mark chunks for destruction outside the lock
        for (uint64_t chunk_id : chunks_to_destroy) {
            _chunks_being_destroyed.insert(chunk_id);
            // Find and mark the chunk as destroyed
            std::shared_lock<std::shared_mutex> chunks_lock(_chunks_lock);
            auto it = _chunks.find(chunk_id);
            if (it != _chunks.end() && it->second)
                it->second->mark_destroyed();
        }
    }

    void scan_for_chunks(const Rect &camera_bounds, const Rect &max_bounds) {
        glm::vec2 tl = glm::vec2(max_bounds.x, max_bounds.y);
        glm::vec2 br = glm::vec2(max_bounds.x + max_bounds.w, max_bounds.y + max_bounds.h);
        glm::vec2 tl_chunk = _camera->world_to_chunk(tl);
        glm::vec2 br_chunk = _camera->world_to_chunk(br);
        for (int y = (int)tl_chunk.y; y <= (int)br_chunk.y; y++)
            for (int x = (int)tl_chunk.x; x <= (int)br_chunk.x; x++) {
                Rect chunk_bounds = Chunk::bounds(x, y);
                if (chunk_bounds.intersects(max_bounds))
                    ensure_chunk(x, y, chunk_bounds.intersects(camera_bounds));
            }
    }

    std::vector<ChunkEvent> release_chunks() {
        std::vector<uint64_t> chunks_to_destroy;
        std::vector<Chunk*> chunks_to_delete;
        std::vector<ChunkEvent> events_to_queue;
        {
            std::unique_lock<std::shared_mutex> lock(_chunks_lock);
            // Iterate through chunks and check if they're marked for destruction
            for (auto it = _chunks.begin(); it != _chunks.end();) {
                uint64_t chunk_id = it->first;
                Chunk* chunk = it->second;
                if (_chunks_being_destroyed.contains(chunk_id)) {
                    std::cout << fmt::format("Releasing chunk at ({}, {})\n", chunk->x(), chunk->y());
                    chunks_to_delete.push_back(chunk);
                    chunks_to_destroy.push_back(chunk_id);
                    events_to_queue.push_back({ChunkEvent::Deleted, chunk->x(), chunk->y()});
                    it = _chunks.erase(it);
                } else
                    ++it;
            }
        }

        // Save chunks to disk and delete them outside of the lock
        for (Chunk* chunk : chunks_to_delete) {
            // Try to save chunk to disk before deletion
            std::string chunk_filepath = _get_chunk_filepath(chunk->x(), chunk->y());
            try {
                if (chunk->serialize(chunk_filepath.c_str()))
                    std::cout << fmt::format("Saved chunk at ({}, {}) to {}\n", chunk->x(), chunk->y(), chunk_filepath);
                else
                    std::cout << fmt::format("Failed to save chunk at ({}, {}) to {}\n", chunk->x(), chunk->y(), chunk_filepath);
            } catch (const std::exception& e) {
                std::cout << fmt::format("Error saving chunk at ({}, {}) to {}: {}\n", chunk->x(), chunk->y(), chunk_filepath, e.what());
            }
            delete chunk;
        }
        // Remove from destroyed set
        for (uint64_t chunk_id : chunks_to_destroy)
            _chunks_being_destroyed.erase(chunk_id);

        return events_to_queue;
    }

    void draw_chunks(sg_pipeline pipeline, bool force_update_mvp) {
        // Collect valid chunks without holding the lock for too long
        std::vector<std::pair<uint64_t, Chunk*>> valid_chunks;
        {
            std::shared_lock<std::shared_mutex> lock(_chunks_lock);
            valid_chunks.reserve(_chunks.size());
            for (const auto& [id, chunk] : _chunks)
                if (chunk != nullptr && !_chunks_being_destroyed.contains(id))
                    valid_chunks.emplace_back(id, chunk);
        }

        for (const auto& [id, chunk] : valid_chunks) {
            // Double-check chunk is still valid (avoid race condition)
            if (_chunks_being_destroyed.contains(id))
                continue;
            sg_apply_pipeline(pipeline);
            chunk->draw(force_update_mvp);
        }
    }
    
    void fire_chunk_events() {
        std::lock_guard<std::mutex> lock(_event_queue_mutex);
        while (!_chunk_event_queue.empty()) {
            ChunkEvent event = _chunk_event_queue.front();
            _chunk_event_queue.pop();
            switch (event.type) {
                case ChunkEvent::Created:
                    call_lua_chunk_event(ChunkEvent::Created, event.x, event.y);
                    break;
                case ChunkEvent::Deleted:
                    call_lua_chunk_event(ChunkEvent::Deleted, event.x, event.y);
                    break;
                case ChunkEvent::VisibilityChanged:
                    call_lua_chunk_event(ChunkEvent::VisibilityChanged, event.x, event.y, event.old_vis, event.new_vis);
                    break;
            }
        }
    }
    
    void queue_events(const std::vector<ChunkEvent>& events) {
        if (!events.empty()) {
            std::lock_guard<std::mutex> lock(_event_queue_mutex);
            for (const auto& event : events)
                _chunk_event_queue.push(event);
        }
    }
    
    void register_lua_callback(int event_type, int lua_ref) {
        // Clear any existing callback for this event
        auto it = _chunk_callbacks.find(event_type);
        if (it != _chunk_callbacks.end() && _lua_state)
            luaL_unref(_lua_state, LUA_REGISTRYINDEX, it->second);

        _chunk_callbacks[event_type] = lua_ref;
    }
    
    void unregister_lua_callback(int event_type) {
        auto it = _chunk_callbacks.find(event_type);
        if (it != _chunk_callbacks.end()) {
            if (_lua_state)
                luaL_unref(_lua_state, LUA_REGISTRYINDEX, it->second);
            _chunk_callbacks.erase(it);
        }
    }
    
    void cleanup_lua_callbacks() {
        if (_lua_state) {
            for (auto& [event_type, ref] : _chunk_callbacks)
                luaL_unref(_lua_state, LUA_REGISTRYINDEX, ref);
            _chunk_callbacks.clear();
        }
    }

    std::optional<std::pair<int, int>> random_walkable_tile_in_chunk(int cx, int cy) {
        Chunk* chunk = nullptr;
        uint64_t idx = index(cx, cy);
        {
            std::shared_lock<std::shared_mutex> lock(_chunks_lock);
            auto it = _chunks.find(idx);
            if (it != _chunks.end())
                chunk = it->second;
        }
        if (chunk && chunk->is_filled())
            return chunk->random_walkable_tile();
        return std::nullopt;
    }

    bool is_chunk_loaded(int cx, int cy) {
        uint64_t idx = index(cx, cy);
        std::shared_lock<std::shared_mutex> lock(_chunks_lock);
        auto it = _chunks.find(idx);
        if (it == _chunks.end())
            return false;
        return it->second != nullptr && it->second->is_filled();
    }
    
    void clear() {
        cleanup_lua_callbacks();
        {
            std::unique_lock<std::shared_mutex> lock(_deletion_queue_lock);
            _deletion_queue.clear();
        }
        {
            std::unique_lock<std::shared_mutex> lock(_chunks_lock);
            // Save all remaining chunks before deletion
            for (auto& [id, chunk] : _chunks) {
                if (chunk == nullptr)
                    continue;
                std::string chunk_filepath = _get_chunk_filepath(chunk->x(), chunk->y());
                try {
                    chunk->serialize(chunk_filepath.c_str());
                    std::cout << fmt::format("Saved chunk at ({}, {}) to {} on shutdown\n", chunk->x(), chunk->y(), chunk_filepath);
                } catch (const std::exception& e) {
                    std::cout << fmt::format("Error saving chunk at ({}, {}) to {} on shutdown: {}\n", chunk->x(), chunk->y(), chunk_filepath, e.what());
                }
                delete chunk;
            }
            _chunks.clear();
        }
    }
};
