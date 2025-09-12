//
//  registrar.hpp
//  nice
//
//  Created by George Watson on 12/09/2025.
//

#pragma once

#include <unordered_map>
#include <string>
#include <atomic>
#include <mutex>

template<typename T>
class Registrar {
    std::unordered_map<uint32_t, T*> _asset;
    std::unordered_map<std::string, uint32_t> _paths;
    std::atomic<uint32_t> _next_id{1}; // Start from 1, 0 can be reserved for "no texture"
    mutable std::mutex _lock;

public:
    uint32_t reigster_asset(const std::string& path, T* asset) {
        std::lock_guard<std::mutex> guard(_lock);
        auto it = _paths.find(path);
        if (it != _paths.end())
            return it->second;
        uint32_t id = _next_id++;
        _asset[id] = asset;
        _paths[path] = id;
        return id;
    }

    T* get_asset(uint32_t id) {
        std::lock_guard<std::mutex> guard(_lock);
        auto it = _asset.find(id);
        return it != _asset.end() ? it->second : nullptr;
    }

    uint32_t get_asset_id(const std::string& path) {
        std::lock_guard<std::mutex> guard(_lock);
        auto it = _paths.find(path);
        return it != _paths.end() ? it->second : 0;
    }

    bool has_asset(const std::string& path) {
        std::lock_guard<std::mutex> guard(_lock);
        return _paths.find(path) != _paths.end();
    }

    void clear() {
        std::lock_guard<std::mutex> guard(_lock);
        _asset.clear();
        _paths.clear();
        _next_id.store(1);
    }
};
