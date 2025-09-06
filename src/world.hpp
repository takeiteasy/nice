//
//  world.hpp
//  nice
//
//  Created by George Watson on 18/08/2025.
//

#pragma once

#include "chunk.hpp"
#include "job_queue.hpp"
#include "texture.hpp"
#include "fmt/format.h"
#include <unordered_map>
#include <shared_mutex>
#include <iostream>
#include <queue>
#include <string>
#include <filesystem>
#include "just_zip.h"
#include "flecs.h"
#include "flecs_lua.h"
#include "entity_manager.hpp"
#include "entity.hpp"
#include "imgui.h"
#include "sol/sol_imgui.h"
#include "nice.dat.h"
#include "uuid.h"
#include "sokol/sokol_time.h"
#include "camera.hpp"
#include "input_manager.hpp"
#include "uuid.h"

class World {
    uuid::v4::UUID _id;
    std::unordered_map<uint64_t, Chunk*> _chunks;
    mutable std::shared_mutex _chunks_lock;
    JobQueue<std::pair<int, int>> _create_chunk_queue;
    ThreadSafeSet<uint64_t> _chunks_being_created;
    JobQueue<Chunk*> _build_chunk_queue;
    ThreadSafeSet<uint64_t> _chunks_being_built;
    ThreadSafeSet<uint64_t> _chunks_being_destroyed;
    std::unordered_map<uint64_t, uint64_t> _deletion_queue;
    mutable std::shared_mutex _deletion_queue_lock;

    struct ChunkEvent {
        enum Type { Created, Deleted, VisibilityChanged } type;
        int x, y;
        ChunkVisibility old_vis = ChunkVisibility::OutOfSign;
        ChunkVisibility new_vis = ChunkVisibility::OutOfSign;
    };
    std::mutex _event_queue_mutex;
    std::queue<ChunkEvent> _chunk_event_queue;

    Camera *_camera;
    Texture *_tilemap;
    sg_shader _shader;
    sg_pipeline _pipeline;
    sg_pipeline _renderables_pipeline;

    flecs::world *_world = nullptr;
    lua_State *L = nullptr;
    std::unordered_map<int, int> _chunk_callbacks;

    static void _abort(void) {
        std::cerr << "ECS: ecs_os_abort() was called!\n";
        std::cerr.flush();
    }

    std::string _get_world_directory() {
        std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
        std::filesystem::path world_dir = temp_dir / _id.String();
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

    void _export() {
        try {
            std::string archive_name = _id.String() + ".niceworld";
            std::cout << fmt::format("Creating world archive: {}\n", archive_name);
            zip* archive = zip_open(archive_name.c_str(), "w");
            if (!archive) {
                std::cout << fmt::format("Failed to create archive: {}\n", archive_name);
                return;
            }
            std::string world_dir = _get_world_directory();
            
            // Iterate through all .nicechunk files in the world directory
            for (const auto& entry : std::filesystem::directory_iterator(world_dir))
                if (entry.is_regular_file() && entry.path().extension() == ".nicechunk") {
                    std::string file_path = entry.path().string();
                    std::string filename = entry.path().filename().string();
                    FILE* file = fopen(file_path.c_str(), "rb");
                    if (!file) {
                        std::cout << fmt::format("Failed to open file for archiving: {}\n", file_path);
                        continue;
                    }
                    bool success = zip_append_file_ex(archive, file_path.c_str(), filename.c_str(), file, 6); // compression level 6
                    fclose(file);
                    if (success)
                        std::cout << fmt::format("Added {} to archive\n", filename);
                    else
                        std::cout << fmt::format("Failed to add {} to archive\n", filename);
                }
            
            zip_close(archive);
            std::cout << fmt::format("World archive created successfully: {}\n", archive_name);
            
            // Clean up the temporary directory after successful archiving
            std::filesystem::remove_all(world_dir);
            std::cout << fmt::format("Cleaned up temporary directory: {}\n", world_dir);
        } catch (const std::exception& e) {
            std::cout << fmt::format("Error creating world archive: {}\n", e.what());
        }
    }

    bool _import(const std::string& archive_path) {
        try {
            std::cout << fmt::format("Loading world from archive: {}\n", archive_path);
            zip* archive = zip_open(archive_path.c_str(), "r");
            if (!archive) {
                std::cout << fmt::format("Failed to open archive: {}\n", archive_path);
                return false;
            }
            std::string world_dir = _get_world_directory();
            unsigned count = zip_count(archive);
            std::cout << fmt::format("Archive contains {} files\n", count);
            
            for (unsigned i = 0; i < count; i++) {
                char* filename = zip_name(archive, i);
                if (filename && strstr(filename, ".nicechunk")) {
                    std::string output_path = (std::filesystem::path(world_dir) / filename).string();
                    FILE* output_file = fopen(output_path.c_str(), "wb");
                    if (!output_file) {
                        std::cout << fmt::format("Failed to create output file: {}\n", output_path);
                        continue;
                    }
                    bool success = zip_extract_file(archive, i, output_file);
                    fclose(output_file);
                    if (success)
                        std::cout << fmt::format("Extracted {} from archive\n", filename);
                    else
                        std::cout << fmt::format("Failed to extract {} from archive\n", filename);
                }
            }
            
            zip_close(archive);
            std::cout << fmt::format("World loaded successfully from archive\n");
        } catch (const std::exception& e) {
            std::cout << fmt::format("Error loading world from archive: {}\n", e.what());
            return false;
        }

        // Extract UUID from filename if possible
        std::filesystem::path p;
        std::string filename = std::filesystem::path(archive_path).filename().string();
        _id = filename.substr(0, filename.find('.'));
        // Loop through extracted chunk files and queue them
        std::string world_dir = _get_world_directory();
        for (const auto& entry : std::filesystem::directory_iterator(world_dir))
            if (entry.is_regular_file() && entry.path().extension() == ".nicechunk") {
                std::string file_path = entry.path().string();
                std::string fname = entry.path().filename().string();
                try {
                    uint64_t chunk_index = std::stoull(fname.substr(0, fname.find('.')));
                    auto [x, y] = unindex(chunk_index);
                    ensure_chunk(x, y, false);
                } catch (const std::exception& e) {
                    std::cout << fmt::format("Failed to parse chunk filename {}: {}\n", fname, e.what());
                    return false;
                }
            }
        return true;
    }

    static void _log(int32_t level, const char *file, int32_t line, const char *msg) {
#if 0
        FILE *stream;
        if (level >= 0)
            stream = stdout;
        else
            stream = stderr;

        if (level >= 0) {
            if (ecs_os_api.flags_ & EcsOsApiLogWithColors)
                fputs(ECS_MAGENTA, stream);
            fputs("info", stream);
        } else if (level == -2) {
            if (ecs_os_api.flags_ & EcsOsApiLogWithColors)
                fputs(ECS_YELLOW, stream);
            fputs("warning", stream);
        } else if (level == -3) {
            if (ecs_os_api.flags_ & EcsOsApiLogWithColors)
                fputs(ECS_RED, stream);
            fputs("error", stream);
        } else if (level == -4) {
            if (ecs_os_api.flags_ & EcsOsApiLogWithColors)
                fputs(ECS_RED, stream);
            fputs("fatal", stream);
        }

        if (ecs_os_api.flags_ & EcsOsApiLogWithColors)
            fputs(ECS_NORMAL, stream);
        fputs(": ", stream);

        if (level >= 0)
            if (ecs_os_api.log_indent_) {
                char indent[32];
                int i;
                for (i = 0; i < ecs_os_api.log_indent_; i ++) {
                    indent[i * 2] = '|';
                    indent[i * 2 + 1] = ' ';
                }
                indent[i * 2] = '\0';
                fputs(indent, stream);
            }

        if (level < 0) {
            if (file && line)
                fprintf(stream, "%s(%d): ", file, line);
            else if(file) {
                const char *file_ptr = strrchr(file, '/');
                if (!file_ptr)
                    file_ptr = strrchr(file, '\\');
                if (file_ptr)
                    file = file_ptr + 1;

                fputs(file, stream);
                fputs(": ", stream);
            }
            else if(line)
                fprintf(stream, "(%d): ", line);
        }

        fputs(msg, stream);
        fputs("\n", stream);
#endif
    }

    static World* get_world_from_lua(lua_State* L) {
        lua_getfield(L, LUA_REGISTRYINDEX, "__world__");
        World* world = static_cast<World*>(lua_touserdata(L, -1));
        lua_pop(L, 1); // Remove the userdata from stack
        if (!world) {
            luaL_error(L, "World instance not found");
            return nullptr;
        }
        return world;
    }

    static const LuaEntity* get_entity_from_lua(lua_State* L) {
        World* world = get_world_from_lua(L);
        if (!world)
            throw std::runtime_error("Failed to get World instance from Lua");
        uint64_t entity_id = static_cast<uint64_t>(luaL_checkinteger(L, 1));
        flecs::entity entity = world->_world->entity(entity_id);
        const LuaEntity* entity_data = entity.get<LuaEntity>();
        if (!entity_data)
            throw std::runtime_error("Entity does not have LuaEntity component");
        return entity_data;
    }

    static LuaEntity* get_mutable_entity_from_lua(lua_State* L) {
        World* world = get_world_from_lua(L);
        if (!world)
            throw std::runtime_error("Failed to get World instance from Lua");
        uint64_t entity_id = static_cast<uint64_t>(luaL_checkinteger(L, 1));
        flecs::entity entity = world->_world->entity(entity_id);
        LuaEntity* entity_data = entity.get_mut<LuaEntity>();
        if (!entity_data)
            throw std::runtime_error("Entity does not have LuaEntity component");
        return entity_data;
    }

    static flecs::entity get_flecs_entity_from_lua(lua_State* L) {
        World* world = get_world_from_lua(L);
        if (!world)
            throw std::runtime_error("Failed to get World instance from Lua");
        uint64_t entity_id = static_cast<uint64_t>(luaL_checkinteger(L, 1));
        flecs::entity entity = world->_world->entity(entity_id);
        const LuaEntity* entity_data = entity.get<LuaEntity>();
        if (!entity_data)
            throw std::runtime_error("Entity does not have LuaEntity component");
        return entity;
    }

    void call_lua_chunk_event(ChunkEvent::Type event_type, int x, int y, ChunkVisibility old_vis = ChunkVisibility::OutOfSign, ChunkVisibility new_vis = ChunkVisibility::OutOfSign) {
        auto it = _chunk_callbacks.find(static_cast<int>(event_type));
        if (it == _chunk_callbacks.end())
            return;
        lua_rawgeti(L, LUA_REGISTRYINDEX, it->second); // Get the callback function
        if (lua_isfunction(L, -1)) {
            lua_pushinteger(L, x);
            lua_pushinteger(L, y);
            if (event_type == ChunkEvent::VisibilityChanged) {
                lua_pushinteger(L, static_cast<int>(old_vis));
                lua_pushinteger(L, static_cast<int>(new_vis));
                if (lua_pcall(L, 4, 0, 0) != LUA_OK) {
                    const char* error_msg = lua_tostring(L, -1);
                    std::cout << fmt::format("Error in chunk callback for type {}: {}\n", static_cast<int>(event_type), error_msg);
                    lua_pop(L, 1); // Remove error message
                }
            } else
                if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
                    const char* error_msg = lua_tostring(L, -1);
                    std::cout << fmt::format("Error in chunk callback for type {}: {}\n", static_cast<int>(event_type), error_msg);
                    lua_pop(L, 1); // Remove error message
                }
        } else
            lua_pop(L, 1); // Remove non-function from stack
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
            _create_chunk_queue.enqueue_priority({x, y});
        else
            _create_chunk_queue.enqueue({x, y});
    }

    void update_chunk(Chunk *chunk, const Rect &camera_bounds, const Rect &max_bounds, 
                      std::vector<std::pair<uint64_t, bool>>& deletion_updates,
                      std::vector<ChunkEvent>& events) {
        if (!chunk->is_ready())
            return;

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

    void update_chunks() {
        Rect max_bounds = _camera->max_bounds();
        Rect bounds = _camera->bounds();

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
        for (auto chunk : chunks_to_update)
            update_chunk(chunk, bounds, max_bounds, deletion_updates, events);

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

    void scan_for_chunks() {
        Rect max_bounds = _camera->max_bounds();
        Rect bounds = _camera->bounds();
        glm::vec2 tl = glm::vec2(max_bounds.x, max_bounds.y);
        glm::vec2 br = glm::vec2(max_bounds.x + max_bounds.w, max_bounds.y + max_bounds.h);
        glm::vec2 tl_chunk = _camera->world_to_chunk(tl);
        glm::vec2 br_chunk = _camera->world_to_chunk(br);
        for (int y = (int)tl_chunk.y; y <= (int)br_chunk.y; y++)
            for (int x = (int)tl_chunk.x; x <= (int)br_chunk.x; x++) {
                Rect chunk_bounds = Chunk::bounds(x, y);
                if (chunk_bounds.intersects(max_bounds))
                    ensure_chunk(x, y, chunk_bounds.intersects(bounds));
            }
    }

    void release_chunks() {
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

        // Queue events outside of chunks lock to avoid potential deadlock
        if (!events_to_queue.empty()) {
            std::lock_guard<std::mutex> lock(_event_queue_mutex);
            for (const auto& event : events_to_queue) {
                _chunk_event_queue.push(event);
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

    void draw_chunks() {
        // Collect valid chunks without holding the lock for too long
        std::vector<std::pair<uint64_t, Chunk*>> valid_chunks;
        {
            std::shared_lock<std::shared_mutex> lock(_chunks_lock);
            valid_chunks.reserve(_chunks.size());
            for (const auto& [id, chunk] : _chunks)
                if (chunk != nullptr && !_chunks_being_destroyed.contains(id))
                    valid_chunks.emplace_back(id, chunk);
        }

        bool force_update_mvp = _camera->is_dirty();
        for (const auto& [id, chunk] : valid_chunks) {
            // Double-check chunk is still valid (avoid race condition)
            if (_chunks_being_destroyed.contains(id))
                continue;
//            if (chunk->id() != 0)
//                continue;
            sg_apply_pipeline(_pipeline);
            chunk->draw(force_update_mvp);
        }
    }

public:
    World(Camera *camera, const char *path = nullptr): _camera(camera), _id(uuid::v4::UUID::New()),
    _create_chunk_queue([&](std::pair<int, int> coords) {
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
        
        // Queue event outside of any locks to avoid deadlocks
        {
            std::lock_guard<std::mutex> lock(_event_queue_mutex);
            _chunk_event_queue.push({ChunkEvent::Created, x, y});
        }
        _build_chunk_queue.enqueue(chunk);
    }),
    _build_chunk_queue([&](Chunk *chunk) {
        chunk->build();
        std::cout << fmt::format("Chunk at ({}, {}) finished building\n", chunk->x(), chunk->y());

        // Remove from being built set after successful build
        uint64_t idx = chunk->id();
        _chunks_being_built.erase(idx);
    }) {
        _shader = sg_make_shader(basic_shader_desc(sg_query_backend()));
        sg_pipeline_desc desc = {
            .shader = _shader,
            .layout = {
                .buffers[0].stride = sizeof(ChunkVertex),
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
            .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8
        };
        _pipeline = sg_make_pipeline(&desc);
        desc.colors[0] = {
            .pixel_format = SG_PIXELFORMAT_RGBA8,
            .blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .src_factor_alpha = SG_BLENDFACTOR_ONE,
                .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
            }
        };
        _renderables_pipeline = sg_make_pipeline(&desc);
        _tilemap = $Assets.get<Texture>("test/tilemap.exploded");

        if (path != nullptr)
            if (!_import(path))
                throw std::runtime_error("Failed to import world from archive");

        // Initialize world directory and print its location
        std::string world_dir = _get_world_directory();
        std::cout << fmt::format("World initialized with UUID: {}\n", _id.String());
        std::cout << fmt::format("Chunk save directory: {}\n", world_dir);

        ecs_os_set_api_defaults();
        ecs_os_api_t os_api = ecs_os_api;
        os_api.abort_ = _abort;
        os_api.log_ = _log;
        ecs_os_set_api(&os_api);
        ecs_log_enable_colors(false);

#pragma region Lua Bindings
        _world = new flecs::world();
        $Entities.set_world(_world);
        // FlecsLua still uses C API for import
        ECS_IMPORT(_world->c_ptr(), FlecsLua);
        // Import C++ modules
#define X(MODULE) _world->import<MODULE>();
        MODULES
#undef X
        L = ecs_lua_get_state(_world->c_ptr());
        ecs_assert(L != NULL, ECS_INTERNAL_ERROR, NULL);

        luaL_openlibs(L);
        lua_getglobal(L, "package");
        lua_getfield(L, -1, "path");
        std::string current_path = lua_tostring(L, -1);
        std::string new_path = current_path + ";./scripts/?.lua";
        lua_pop(L, 1);
        lua_pushstring(L, new_path.c_str());
        lua_setfield(L, -2, "path");
        lua_pop(L, 1);

        sol::state_view lua(L);
        lua["imgui"] = imgui::load(static_cast<sol::state&>(lua));

        // Register texture function for Lua
        lua_register(L, "get_texture_id", [](lua_State* L) -> int {
            const char* path = luaL_checkstring(L, 1);
            uint32_t texture_id = $Entities.register_texture(path);
            lua_pushinteger(L, texture_id);
            return 1; // Return the texture ID
        });

        lua_register(L, "framebuffer_width", [](lua_State* L) -> int {
            lua_pushinteger(L, framebuffer_width());
            return 1;
        });

        lua_register(L, "framebuffer_height", [](lua_State* L) -> int {
            lua_pushinteger(L, framebuffer_height());
            return 1;
        });

        lua_register(L, "frame_duration", [](lua_State* L) -> int {
            lua_pushnumber(L, sapp_frame_duration());
            return 1;
        });

        lua_register(L, "framebuffer_resize", [](lua_State* L) -> int {
            int width = static_cast<int>(luaL_checkinteger(L, 1));
            int height = static_cast<int>(luaL_checkinteger(L, 2));
            framebuffer_resize(width, height);
            return 0; // No return value
        });

        lua_register(L, "chunk_index", [](lua_State* L) -> int {
            int x = static_cast<int>(luaL_checkinteger(L, 1));
            int y = static_cast<int>(luaL_checkinteger(L, 2));
            uint64_t result = index(x, y);
            lua_pushinteger(L, result);
            return 1;
        });

        lua_register(L, "chunk_unindex", [](lua_State* L) -> int {
            uint64_t i = luaL_checkinteger(L, 1);
            auto coords = unindex(i);
            lua_pushinteger(L, coords.first);
            lua_pushinteger(L, coords.second);
            return 2; // Return two values: x, y
        });

        lua_pushlightuserdata(L, this);
        lua_setfield(L, LUA_REGISTRYINDEX, "__world__");
        
        lua_register(L, "poisson", [](lua_State* L) -> int {
            World* world = get_world_from_lua(L);
            if (!world)
                return 0;
            int num_args = lua_gettop(L);
            if (num_args < 3) {
                luaL_error(L, "poisson requires at least 3 arguments: chunk_x, chunk_y, radius");
                return 0;
            }

            // Get chunk coordinates (can be either x,y or index)
            long long chunk_x;
            long long chunk_y;
            if (num_args >= 4 && lua_isinteger(L, 1) && lua_isinteger(L, 2) && lua_isnumber(L, 3)) {
                // Format: chunk_x, chunk_y, radius, [k], [invert], [region_x], [region_y], [region_w], [region_h]
                chunk_x = luaL_checkinteger(L, 1);
                chunk_y = luaL_checkinteger(L, 2);
            } else {
                // Format: chunk_index, radius, [k], [invert], [region_x], [region_y], [region_w], [region_h]
                uint64_t chunk_index = luaL_checkinteger(L, 1);
                auto coords = unindex(chunk_index);
                chunk_x = coords.first;
                chunk_y = coords.second;
                // Shift all other arguments by 1
                for (int i = 2; i <= num_args; i++)
                    lua_pushvalue(L, i);
                lua_remove(L, 1); // Remove the index
                num_args--; // Adjust argument count
            }

            float radius = luaL_checknumber(L, (num_args >= 4) ? 3 : 2);
            long long k = 30;
            bool invert = false;
            Rect region = Rect(0, 0, CHUNK_WIDTH, CHUNK_HEIGHT);
            int arg_offset = (num_args >= 4) ? 4 : 3; // Starting position for optional args
            if (num_args > arg_offset)
                k = luaL_checkinteger(L, arg_offset);
            if (num_args > arg_offset + 1)
                invert = lua_toboolean(L, arg_offset + 1);
            if (num_args > arg_offset + 2)
                region.x = static_cast<int>(luaL_checkinteger(L, arg_offset + 2));
            if (num_args > arg_offset + 3)
                region.y = static_cast<int>(luaL_checkinteger(L, arg_offset + 3));
            if (num_args > arg_offset + 4)
                region.w = static_cast<int>(luaL_checkinteger(L, arg_offset + 4));
            if (num_args > arg_offset + 5)
                region.h = static_cast<int>(luaL_checkinteger(L, arg_offset + 5));

            // Find the chunk
            uint64_t chunk_id = index(static_cast<int>(chunk_x), static_cast<int>(chunk_y));
            Chunk* chunk = nullptr;
            {
                std::shared_lock<std::shared_mutex> lock(world->_chunks_lock);
                auto it = world->_chunks.find(chunk_id);
                if (it != world->_chunks.end())
                    chunk = it->second;
            }
            
            if (!chunk || !chunk->is_filled()) {
                lua_pushnil(L);
                return 1; // Return nil if chunk not found or not filled
            }

            std::vector<glm::vec2> points = chunk->poisson(radius, static_cast<int>(k), invert, true, CHUNK_SIZE / 4, region);
            lua_createtable(L, static_cast<int>(points.size()), 0);
            for (size_t i = 0; i < points.size(); i++) {
                lua_createtable(L, 2, 0);
                lua_pushnumber(L, points[i].x);
                lua_rawseti(L, -2, 1);
                lua_pushnumber(L, points[i].y);
                lua_rawseti(L, -2, 2);
                lua_rawseti(L, -2, i + 1);
            }
            return 1;
        });

        lua_register(L, "camera_position", [](lua_State *L) -> int {
            World* world = get_world_from_lua(L);
            if (!world)
                return 0;
            glm::vec2 pos = world->_camera->position();
            lua_pushnumber(L, pos.x);
            lua_pushnumber(L, pos.y);
            return 2;
        });

        lua_register(L, "camera_set_position", [](lua_State *L) -> int {
            float x = static_cast<int>(luaL_checknumber(L, 1));
            float y = static_cast<int>(luaL_checknumber(L, 2));
            // Get World instance from Lua registry
            World* world = get_world_from_lua(L);
            if (!world)
                return 0;
            world->_camera->set_position(glm::vec2(x, y));
            return 0;
        });

        lua_register(L, "camera_move", [](lua_State *L) -> int {
            float x = static_cast<int>(luaL_checknumber(L, 1));
            float y = static_cast<int>(luaL_checknumber(L, 2));
            // Get World instance from Lua registry
            World* world = get_world_from_lua(L);
            if (!world)
                return 0;
            world->_camera->move_by(glm::vec2(x, y));
            return 0;
        });

        lua_register(L, "camera_zoom", [](lua_State *L) -> int {
            World* world = get_world_from_lua(L);
            if (!world)
                return 0;
            lua_pushnumber(L, world->_camera->zoom());
            return 1;
        });

        lua_register(L, "camera_set_zoom", [](lua_State *L) -> int {
            float z = static_cast<float>(luaL_checknumber(L, 1));
            World *world = get_world_from_lua(L);
            if (!world)
                return 0;
            world->_camera->set_zoom(z);
            return 0;
        });

        lua_register(L, "camera_zoom_by", [](lua_State *L) -> int {
            float delta = static_cast<float>(luaL_checknumber(L, 1));
            World *world = get_world_from_lua(L);
            if (!world)
                return 0;
            world->_camera->zoom_by(delta);
            return 0;
        });

        lua_register(L, "camera_bounds", [](lua_State *L) -> int {
            World* world = get_world_from_lua(L);
            if (!world)
                return 0;
            Rect bounds = world->_camera->bounds();
            lua_pushnumber(L, bounds.x);
            lua_pushnumber(L, bounds.y);
            lua_pushnumber(L, bounds.w);
            lua_pushnumber(L, bounds.h);
            return 4;
        });

        lua_register(L, "world_to_screen", [](lua_State *L) -> int {
            float world_x = static_cast<float>(luaL_checknumber(L, 1));
            float world_y = static_cast<float>(luaL_checknumber(L, 2));
            World* world = get_world_from_lua(L);
            if (!world)
                return 0;
            glm::vec2 screen_pos = world->_camera->world_to_screen(glm::vec2(world_x, world_y));
            lua_pushnumber(L, screen_pos.x);
            lua_pushnumber(L, screen_pos.y);
            return 2;
        });

        lua_register(L, "screen_to_world", [](lua_State *L) -> int {
            float screen_x = static_cast<float>(luaL_checknumber(L, 1));
            float screen_y = static_cast<float>(luaL_checknumber(L, 2));
            World* world = get_world_from_lua(L);
            if (!world)
                return 0;
            glm::vec2 world_pos = world->_camera->screen_to_world(glm::vec2(screen_x, screen_y));
            lua_pushnumber(L, world_pos.x);
            lua_pushnumber(L, world_pos.y);
            return 2;
        });

        lua_register(L, "world_to_tile", [](lua_State *L) -> int {
            float world_x = static_cast<float>(luaL_checknumber(L, 1));
            float world_y = static_cast<float>(luaL_checknumber(L, 2));
            glm::vec2 tile = Camera::world_to_tile(glm::vec2(world_x, world_y));
            lua_pushnumber(L, tile.x);
            lua_pushnumber(L, tile.y);
            return 2;
        });

        lua_register(L, "world_to_chunk", [](lua_State *L) -> int {
            float world_x = static_cast<float>(luaL_checknumber(L, 1));
            float world_y = static_cast<float>(luaL_checknumber(L, 2));
            glm::vec2 chunk = Camera::world_to_chunk(glm::vec2(world_x, world_y));
            lua_pushnumber(L, chunk.x);
            lua_pushnumber(L, chunk.y);
            return 2;
        });

        lua_register(L, "chunk_to_world", [](lua_State *L) -> int {
            int chunk_x = static_cast<int>(luaL_checkinteger(L, 1));
            int chunk_y = static_cast<int>(luaL_checkinteger(L, 2));
            glm::vec2 world_pos = Camera::chunk_to_world(chunk_x, chunk_y);
            lua_pushnumber(L, world_pos.x);
            lua_pushnumber(L, world_pos.y);
            return 2;
        });

        lua_register(L, "tile_to_world", [](lua_State *L) -> int {
            glm::vec2 world_pos;
            flecs::entity e = get_flecs_entity_from_lua(L);
            // entity, tile_x, tile_y
            const LuaChunk *chunk = e.get<LuaChunk>();
            int tile_x = static_cast<int>(luaL_checkinteger(L, 2));
            int tile_y = static_cast<int>(luaL_checkinteger(L, 3));
            world_pos = Camera::tile_to_world(chunk->x, chunk->y, tile_x, tile_y);
            lua_pushnumber(L, world_pos.x);
            lua_pushnumber(L, world_pos.y);
            return 2;
        });

        lua_register(L, "set_entity_world_position", [](lua_State *L) -> int {
            flecs::entity e = get_flecs_entity_from_lua(L);
            LuaEntity* entity_data = e.get_mut<LuaEntity>();
            if (!entity_data)
                throw std::runtime_error("Entity is missing LuaEntity component");
            LuaChunk *chunk = e.get_mut<LuaChunk>();
            if (!chunk)
                throw std::runtime_error("Entity is missing LuaChunk component");
            entity_data->x = static_cast<float>(luaL_checknumber(L, 2));
            entity_data->y = static_cast<float>(luaL_checknumber(L, 3));
            glm::vec2 _chunk = Camera::world_to_chunk({entity_data->x, entity_data->y});
            chunk->x = static_cast<int>(_chunk.x);
            chunk->y = static_cast<int>(_chunk.y);
            return 0;
        });

        lua_register(L, "get_entity_world_position", [](lua_State *L) -> int {
            const LuaEntity* entity_data = get_entity_from_lua(L);
            lua_pushnumber(L, entity_data->x);
            lua_pushnumber(L, entity_data->y);
            return 2;
        });

        lua_register(L, "set_entity_position", [](lua_State *L) -> int {
            flecs::entity e = get_flecs_entity_from_lua(L);
            LuaEntity* entity_data = e.get_mut<LuaEntity>();
            if (!entity_data)
                throw std::runtime_error("Entity is missing LuaEntity component");
            LuaChunk *chunk = e.get_mut<LuaChunk>();
            if (!chunk)
                throw std::runtime_error("Entity is missing LuaChunk component");
            int chunk_x = static_cast<int>(luaL_checkinteger(L, 2));
            int chunk_y = static_cast<int>(luaL_checkinteger(L, 3));
            int tile_x = static_cast<int>(luaL_checkinteger(L, 4));
            int tile_y = static_cast<int>(luaL_checkinteger(L, 5));
            glm::vec2 world_pos = Camera::tile_to_world(chunk_x, chunk_y, tile_x, tile_y);
            entity_data->x = world_pos.x;
            entity_data->y = world_pos.y;
            chunk->x = chunk_x;
            chunk->y = chunk_y;
            return 0;
        });

        lua_register(L, "get_entity_position", [](lua_State *L) -> int {
            const LuaEntity* entity_data = get_entity_from_lua(L);
            glm::vec2 chunk = Camera::world_to_chunk({entity_data->x, entity_data->y});
            glm::vec2 tile = Camera::world_to_tile({entity_data->x, entity_data->y});
            lua_pushinteger(L, static_cast<int>(chunk.x));
            lua_pushinteger(L, static_cast<int>(chunk.y));
            lua_pushinteger(L, static_cast<int>(tile.x));
            lua_pushinteger(L, static_cast<int>(tile.y));
            return 4;
        });

        lua_register(L, "set_entity_size", [](lua_State *L) -> int {
            LuaEntity* entity_data = get_mutable_entity_from_lua(L);
            switch (lua_gettop(L)) {
                case 2:
                    entity_data->width = entity_data->height = static_cast<float>(luaL_checknumber(L, 2));
                    break;
                case 3:
                    entity_data->width = static_cast<float>(luaL_checknumber(L, 2));
                    entity_data->height = static_cast<float>(luaL_checknumber(L, 3));
                    break;
                default:
                    luaL_error(L, "set_entity_size expects either (entity, size) or (entity, width, height)");
            }
            return 0;
        });

        lua_register(L, "get_entity_size", [](lua_State *L) -> int {
            const LuaEntity* entity_data = get_entity_from_lua(L);
            lua_pushnumber(L, entity_data->width);
            lua_pushnumber(L, entity_data->height);
            return 2;
        });

        lua_register(L, "set_entity_z", [](lua_State *L) -> int {
            LuaEntity* entity_data = get_mutable_entity_from_lua(L);
            entity_data->z_index = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        });

        lua_register(L, "get_entity_z", [](lua_State *L) -> int {
            const LuaEntity* entity_data = get_entity_from_lua(L);
            lua_pushnumber(L, entity_data->z_index);
            return 1;
        });

        lua_register(L, "set_entity_rotation", [](lua_State *L) -> int {
            LuaEntity* entity_data = get_mutable_entity_from_lua(L);
            entity_data->rotation = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        });

        lua_register(L, "get_entity_rotation", [](lua_State *L) -> int {
            const LuaEntity* entity_data = get_entity_from_lua(L);
            lua_pushnumber(L, entity_data->rotation);
            return 1;
        });

        lua_register(L, "set_entity_scale", [](lua_State *L) -> int {
            LuaEntity* entity_data = get_mutable_entity_from_lua(L);
            switch (lua_gettop(L)) {
                case 2:
                    entity_data->scale_x = entity_data->scale_y = static_cast<float>(luaL_checknumber(L, 2));
                    break;
                case 3:
                    entity_data->scale_x = static_cast<float>(luaL_checknumber(L, 2));
                    entity_data->scale_y = static_cast<float>(luaL_checknumber(L, 3));
                    break;
                default:
                    luaL_error(L, "set_entity_scale expects either (entity, scale) or (entity, scale_x, scale_y)");
            }
            return 0;
        });

        lua_register(L, "get_entity_scale", [](lua_State *L) -> int {
            const LuaEntity* entity_data = get_entity_from_lua(L);
            lua_pushnumber(L, entity_data->scale_x);
            lua_pushnumber(L, entity_data->scale_y);
            return 2;
        });

        lua_register(L, "set_entity_clip", [](lua_State *L) -> int {
            LuaEntity* entity_data = get_mutable_entity_from_lua(L);
            if (lua_gettop(L) == 1) {
                entity_data->clip_x = 0;
                entity_data->clip_y = 0;
                entity_data->clip_width = 0;
                entity_data->clip_height = 0;
            } else {
                entity_data->clip_x = static_cast<int>(luaL_checkinteger(L, 2));
                entity_data->clip_y = static_cast<int>(luaL_checkinteger(L, 3));
                entity_data->clip_width = static_cast<int>(luaL_checkinteger(L, 4));
                entity_data->clip_height = static_cast<int>(luaL_checkinteger(L, 5));
            }
            return 0;
        });

        lua_register(L, "set_entity_clip_size", [](lua_State *L) -> int {
            LuaEntity* entity_data = get_mutable_entity_from_lua(L);
            entity_data->clip_width = static_cast<int>(luaL_checkinteger(L, 2));
            entity_data->clip_height = static_cast<int>(luaL_checkinteger(L, 3));
            return 0;
        });

        lua_register(L, "set_entity_clip_offset", [](lua_State *L) -> int {
            LuaEntity* entity_data = get_mutable_entity_from_lua(L);
            entity_data->clip_x = static_cast<int>(luaL_checkinteger(L, 2));
            entity_data->clip_y = static_cast<int>(luaL_checkinteger(L, 3));
            return 0;
        });

        lua_register(L, "get_entity_clip", [](lua_State *L) -> int {
            const LuaEntity* entity_data = get_entity_from_lua(L);
            lua_newtable(L);
            lua_pushinteger(L, entity_data->clip_x);
            lua_setfield(L, -2, "x");
            lua_pushinteger(L, entity_data->clip_y);
            lua_setfield(L, -2, "y");
            lua_pushinteger(L, entity_data->clip_width);
            lua_setfield(L, -2, "width");
            lua_pushinteger(L, entity_data->clip_height);
            lua_setfield(L, -2, "height");
            return 1;
        });

        lua_register(L, "set_entity_speed", [](lua_State *L) -> int {
            LuaEntity* entity_data = get_mutable_entity_from_lua(L);
            entity_data->speed = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        });

        lua_register(L, "get_entity_speed", [](lua_State *L) -> int {
            const LuaEntity* entity_data = get_entity_from_lua(L);
            lua_pushnumber(L, entity_data->speed);
            return 1;
        });

        lua_register(L, "set_entity_target", [](lua_State *L) -> int {
            flecs::entity entity = get_flecs_entity_from_lua(L);
            int x = static_cast<int>(luaL_checkinteger(L, 2));
            int y = static_cast<int>(luaL_checkinteger(L, 3));
            if (x < 0 || y < 0 || x >= CHUNK_WIDTH || y >= CHUNK_HEIGHT)
                throw std::runtime_error("Target coordinates must be within chunk bounds");
            LuaChunk *lchunk = entity.get_mut<LuaChunk>();
            if (!lchunk)
                throw std::runtime_error("Entity is missing LuaChunk component");
            World* world = get_world_from_lua(L);
            if (!world)
                throw std::runtime_error("Internal Error: World instance not found in Lua registry");
            world->get_chunk(lchunk->x, lchunk->y, [&](Chunk* chunk) {
                if (chunk->is_walkable(x, y, false))
                    entity.set<LuaTarget>({x, y});
            });
            return 0;
        });

        lua_register(L, "get_entity_target", [](lua_State *L) -> int {
            flecs::entity entity = get_flecs_entity_from_lua(L);
            const LuaTarget *target = entity.get<LuaTarget>();
            if (target) {
                lua_pushnumber(L, target->x);
                lua_pushnumber(L, target->y);
                return 2;
            } else {
                lua_pushnil(L);
                lua_pushnil(L);
                return 2;
            }
        });

        lua_register(L, "clear_entity_target", [](lua_State *L) -> int {
            flecs::entity entity = get_flecs_entity_from_lua(L);
            entity.remove<LuaTarget>();
            return 0;
        });

        lua_register(L, "entity_has_target", [](lua_State *L) -> int {
            flecs::entity entity = get_flecs_entity_from_lua(L);
            lua_pushboolean(L, entity.has<LuaTarget>());
            return 1;
        });

        lua_register(L, "get_entity_bounds", [](lua_State *L) -> int {
            const LuaEntity* entity_data = get_entity_from_lua(L);
            Rect bounds = EntityManager::entity_bounds(*entity_data);
            lua_newtable(L);
            lua_pushinteger(L, bounds.x);
            lua_setfield(L, -2, "x");
            lua_pushinteger(L, bounds.y);
            lua_setfield(L, -2, "y");
            lua_pushinteger(L, bounds.w);
            lua_setfield(L, -2, "width");
            lua_pushinteger(L, bounds.h);
            lua_setfield(L, -2, "height");
            return 1;
        });

        lua_register(L, "is_entity_visible", [](lua_State *L) -> int {
            const LuaEntity* entity_data = get_entity_from_lua(L);
            World* world = get_world_from_lua(L);
            if (!world)
                throw std::runtime_error("Internal Error: World instance not found in Lua registry");
            Rect entity_bounds = EntityManager::entity_bounds(*entity_data);
            Rect camera_bounds = world->_camera->bounds();
            lua_pushboolean(L, camera_bounds.intersects(entity_bounds));
            return 1;
        });

        // Expose ChunkEvent types to Lua
        lua_newtable(L);
        lua_pushinteger(L, ChunkEvent::Created);
        lua_setfield(L, -2, "created");
        lua_pushinteger(L, ChunkEvent::Deleted);
        lua_setfield(L, -2, "deleted");
        lua_pushinteger(L, ChunkEvent::VisibilityChanged);
        lua_setfield(L, -2, "visibility_changed");
        lua_setglobal(L, "ChunkEventType");

        // Expose ChunkVisibility types to Lua
        lua_newtable(L);
        lua_pushinteger(L, static_cast<int>(ChunkVisibility::OutOfSign));
        lua_setfield(L, -2, "out_of_sign");
        lua_pushinteger(L, static_cast<int>(ChunkVisibility::Visible));
        lua_setfield(L, -2, "visible");
        lua_pushinteger(L, static_cast<int>(ChunkVisibility::Occluded));
        lua_setfield(L, -2, "occluded");
        lua_setglobal(L, "ChunkVisibility");

        // Register chunk callback management functions
        lua_register(L, "register_chunk_callback", [](lua_State *L) -> int {
            if (!lua_isinteger(L, 1) || !lua_isfunction(L, 2)) {
                luaL_error(L, "register_chunk_callback expects (integer, function)");
                return 0;
            }
            
            int event_type = static_cast<int>(luaL_checkinteger(L, 1));
            World* world = get_world_from_lua(L);
            if (!world)
                return 0;

            // Store the function in the registry and get a reference
            lua_pushvalue(L, 2); // Duplicate the function
            int ref = luaL_ref(L, LUA_REGISTRYINDEX);
            
            // Clear any existing callback for this event
            auto& callbacks = world->_chunk_callbacks;
            auto it = callbacks.find(event_type);
            if (it != callbacks.end())
                luaL_unref(L, LUA_REGISTRYINDEX, it->second);

            callbacks[event_type] = ref;
            return 0;
        });

        lua_register(L, "unregister_chunk_callback", [](lua_State *L) -> int {
            if (!lua_isinteger(L, 1)) {
                luaL_error(L, "unregister_chunk_callback expects (integer)");
                return 0;
            }
            
            int event_type = static_cast<int>(luaL_checkinteger(L, 1));
            World* world = get_world_from_lua(L);
            if (!world) return 0;
            
            auto& callbacks = world->_chunk_callbacks;
            auto it = callbacks.find(event_type);
            if (it != callbacks.end()) {
                luaL_unref(L, LUA_REGISTRYINDEX, it->second);
                callbacks.erase(it);
            }
            return 0;
        });

        // Register input functions
        $Input.load_into_lua(L);
#pragma endregion

        if (luaL_loadbuffer(L, reinterpret_cast<const char*>(setup_lua), setup_lua_size, "setup.lua") != LUA_OK ||
            lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK) {
            const char* error_msg = lua_tostring(L, -1);
            fprintf(stderr, "Lua error in setup.lua: %s\n", error_msg);
            lua_pop(L, 1); // Remove error message from stack
            throw std::runtime_error("Internal Error: Failed to execute `setup.lua`");
        }

        GenericAsset *main_lua = $Assets.get<>("main.lua");
        if (main_lua && main_lua->is_valid() && !main_lua->data().empty()) {
            if (luaL_loadbuffer(L, reinterpret_cast<const char*>(main_lua->raw_data()), main_lua->size(), "main.lua") != LUA_OK ||
                lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK) {
                const char* error_msg = lua_tostring(L, -1);
                std::cout << fmt::format("Lua error in main.lua: {}\n", error_msg);
                lua_pop(L, 1); // Remove error message from stack
                throw std::runtime_error("Failed to execute `main.lua`");
            }
        } else
            std::cout << fmt::format("Warning: main.lua not found or invalid, skipping execution\n");
        ecs_assert(L != NULL, ECS_INTERNAL_ERROR, NULL);
    }

    ~World() {
        // Clean up InputManager callbacks before cleaning up chunk callbacks
        $Input.cleanup_lua_callbacks();
        
        // Clean up chunk callback references
        if (L) {
            for (auto& [event_type, ref] : _chunk_callbacks)
                luaL_unref(L, LUA_REGISTRYINDEX, ref);
            _chunk_callbacks.clear();
        }
        $Entities.clear();
        if (_world)
            delete _world;
        if (sg_query_shader_state(_shader) == SG_RESOURCESTATE_VALID)
            sg_destroy_shader(_shader);
        if (sg_query_pipeline_state(_pipeline) == SG_RESOURCESTATE_VALID)
            sg_destroy_pipeline(_pipeline);
        if (sg_query_pipeline_state(_renderables_pipeline) == SG_RESOURCESTATE_VALID)
            sg_destroy_pipeline(_renderables_pipeline);
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
        _export();
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

    bool update(float dt) {
        update_chunks();
        update_deletion_queue();
        scan_for_chunks();
        release_chunks();
        fire_chunk_events();
        bool result = _world->progress(dt);
        if (result) {
            $Entities.finalize(_camera);
            draw_chunks();
            sg_apply_pipeline(_renderables_pipeline);
            $Entities.flush(_camera);
        }
        return result;
    }
};
