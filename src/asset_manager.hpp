//
//  assets.hpp
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

#define $Assets Assets::instance()

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
    T* get(const std::string& key, bool ensure = true, Args&&... args) {
        std::lock_guard<std::mutex> lock(_map_lock);
        auto it = _assets.find(key);
        if (it != _assets.end())
            if (auto* asset = dynamic_cast<T*>(it->second.get()))
                return asset;
        if (!ensure)
            return nullptr;
        auto asset = std::make_unique<T>(std::forward<Args>(args)...);
        T* result = asset.get();
        result->load(key);  // Pass path to load
        _assets[key] = std::move(asset);
        return result;
    };

    void clear() {
        std::lock_guard<std::mutex> lock(_map_lock);
        _assets.clear();
    }
};
