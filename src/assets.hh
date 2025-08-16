//
//  assets.hh
//  rpg
//
//  Created by George Watson on 16/08/2025.
//

#pragma once

#include <iostream>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <string>
#include <optional>

class AssetBase {
public:
    virtual bool load(const std::string& path) = 0;
    virtual void unload() = 0;
    virtual bool is_valid() const = 0;
    virtual ~AssetBase() = default;
};

template<typename Derived>
class Asset : public AssetBase {
private:
    bool cleanup_done = false;  // Prevent double cleanup

public:
    // Forward the interface to the derived class implementation
    bool load(const std::string& path) override {
        return cleanup_done ? false : static_cast<Derived*>(this)->Derived::load(path);  // Direct call to avoid virtual dispatch
    }

    void unload() override {
        if (cleanup_done)
            return;
        static_cast<Derived*>(this)->Derived::unload();  // Direct call to avoid virtual dispatch
        cleanup_done = true;
    }

    bool is_valid() const override {
        return cleanup_done ? false : static_cast<const Derived*>(this)->Derived::is_valid();  // Direct call to avoid virtual dispatch
    }

    // Handle cleanup in Asset destructor where Derived is still valid
    virtual ~Asset() {
        if (cleanup_done)
            return;
        // Use non-virtual calls to avoid issues during destruction
        auto* derived = static_cast<Derived*>(this);
        if (derived->Derived::is_valid())
            derived->Derived::unload();
        cleanup_done = true;
    }
};

class Assets {
private:
    inline static std::unique_ptr<Assets> _instance = nullptr;
    inline static std::once_flag _once;

    std::unordered_map<std::string, std::unique_ptr<AssetBase>> _assets;
    mutable std::mutex _map_lock;

    Assets() = default;
    Assets(const Assets&) = delete;
    Assets& operator=(const Assets&) = delete;

public:
    static Assets& instance() {
        std::call_once(_once, []() {
            _instance = std::unique_ptr<Assets>(new Assets());
        });
        return *_instance;
    }

    template<typename T, typename... Args>
    T* load(const std::string& key, const std::string& path, Args&&... args) {
        std::lock_guard<std::mutex> lock(_map_lock);

        // Check if already exists
        auto it = _assets.find(key);
        if (it != _assets.end())
            if (auto* asset = dynamic_cast<T*>(it->second.get())) {
                if (!asset->is_valid())
                    asset->load(path);  // Pass path to load
                return asset;
            }

        // Create new asset
        auto asset = std::make_unique<T>(std::forward<Args>(args)...);
        T* result = asset.get();
        result->load(path);  // Pass path to load
        _assets[key] = std::move(asset);
        return result;
    }

    template<typename T> T* get(const std::string& key) const {
        std::lock_guard<std::mutex> lock(_map_lock);
        auto it = _assets.find(key);
        if (it != _assets.end())
            return dynamic_cast<T*>(it->second.get());
        return nullptr;
    }

    bool has(const std::string& key) const {
        std::lock_guard<std::mutex> lock(_map_lock);
        auto it = _assets.find(key);
        return it != _assets.end() && it->second->is_valid();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(_map_lock);
        _assets.clear();
    }
};

#define $ASSETS Assets::instance()
