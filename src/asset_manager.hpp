//
//  assets.hpp
//  ice
//
//  Created by George Watson on 16/08/2025.
//

#pragma once

#include <mutex>
#include <memory>
#include <unordered_map>
#include <string>
#include "just_zip.h"

#define $Assets Assets::instance()

class AssetBase {
public:
    virtual bool load(const unsigned char *data, size_t data_size) = 0;
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
    bool load(const unsigned char *data, size_t data_size) override {
        if (cleanup_done)
            return false;
        return static_cast<Derived*>(this)->Derived::load(data, data_size);  // Direct call to avoid virtual dispatch
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
    zip *_archive = nullptr;
    mutable std::mutex _archive_lock;

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

    bool set_archive(const std::string& path) {
        std::unique_lock<std::mutex> lock(_archive_lock);
        if (_archive != nullptr) {
            lock.unlock();
            clear();
            lock.lock();
        }
        bool result = (_archive = zip_open(path.c_str(), "r")) != NULL;
        lock.unlock();
        return result;
    }

    template<typename T, typename... Args>
    T* get(const std::string& key, bool ensure = true, Args&&... args) {
        std::lock_guard<std::mutex> _a_lock(_archive_lock);
        if (!_archive)
            return nullptr;
        std::lock_guard<std::mutex> m_lock(_map_lock);
        auto it = _assets.find(key);
        if (it != _assets.end())
            if (auto* asset = dynamic_cast<T*>(it->second.get()))
                return asset;
        if (!ensure)
            return nullptr;
        auto asset = std::make_unique<T>(std::forward<Args>(args)...);
        T* result = asset.get();
        int index;
        if ((index = zip_find(_archive, key.c_str())) < 0)
            return nullptr;
        size_t file_size = zip_size(_archive, index);
        unsigned char* data = (unsigned char*)std::malloc(file_size);
        zip_extract_data(_archive, index, data, (int)file_size);
        result->load(data, file_size);  // Pass data to load
        std::free(data);
        _assets[key] = std::move(asset);
        return result;
    };

    void clear() {
        std::lock_guard<std::mutex> _m_lock(_map_lock);
        _assets.clear();
        std::lock_guard<std::mutex> _a_lock(_archive_lock);
        if (_archive) {
            zip_close(_archive);
            _archive = nullptr;
        }
    }
};
