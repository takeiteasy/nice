//
//  assets.hpp
//  nice
//
//  Created by George Watson on 16/08/2025.
//

#pragma once

#include <mutex>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include "just_zip.h"
#include "global.hpp"

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

    virtual std::string asset_extension() const {
        return "";
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

class GenericAsset: public Asset<GenericAsset> {
    std::vector<unsigned char> _data;

public:
    bool load(const unsigned char *data, size_t data_size) override {
        _data.resize(data_size);
        std::copy(data, data + data_size, _data.begin());
        return true;
    }

    void unload() override {
        _data.clear();
    }

    bool is_valid() const override {
        return !_data.empty();
    }

    std::string asset_extension() const override {
        return "";
    }

    const std::vector<unsigned char>& data() const {
        return _data;
    }

    const unsigned char* raw_data() const {
        return _data.data();
    }

    size_t size() const {
        return _data.size();
    }
};

class Assets: public Global<Assets> {
    std::unordered_map<std::string, std::unique_ptr<AssetBase>> _assets;
    mutable std::mutex _map_lock;
    zip *_archive = nullptr;
    mutable std::mutex _archive_lock;

public:
    Assets() = default;  // Add explicit default constructor

    bool set_archive(const std::string& path) {
        // To avoid deadlock, first close any existing archive without calling clear()
        zip* old_archive = nullptr;
        {
            std::unique_lock<std::mutex> lock(_archive_lock);
            old_archive = _archive;
            _archive = nullptr;
        }
        
        // Close old archive outside of any locks
        if (old_archive != nullptr)
            zip_close(old_archive);

        // Clear assets separately to avoid lock ordering issues
        {
            std::lock_guard<std::mutex> _m_lock(_map_lock);
            _assets.clear();
        }
        
        // Now set the new archive
        std::unique_lock<std::mutex> lock(_archive_lock);
        bool result = (_archive = zip_open(path.c_str(), "r")) != NULL;
        return result;
    }
    
    template<typename T=GenericAsset, typename... Args>
    T* get(const std::string& key, bool ensure = true, Args&&... args) {
        std::lock_guard<std::mutex> _a_lock(_archive_lock);
        if (!_archive)
            return nullptr;
        std::lock_guard<std::mutex> m_lock(_map_lock);
        std::string ext = T().asset_extension();
        std::string final_key = key;
        if (key.substr(key.length() - ext.length()) != ext)
            final_key += ext;
        auto it = _assets.find(final_key);
        if (it != _assets.end())
            if (auto* asset = dynamic_cast<T*>(it->second.get()))
                return asset;
        if (!ensure)
            return nullptr;
        auto asset = std::make_unique<T>(std::forward<Args>(args)...);
        T* result = asset.get();
        size_t file_size = 0;
        unsigned char *data = nullptr;
        if (_archive != nullptr) {
            int index;
            if ((index = zip_find(_archive, final_key.c_str())) < 0)
                return nullptr;
            file_size = zip_size(_archive, index);
            data = (unsigned char*)std::malloc(file_size);
            zip_extract_data(_archive, index, data, (int)file_size);
        } else {
            FILE* file = fopen(final_key.c_str(), "rb");
            if (!file)
                return nullptr;
            fseek(file, 0, SEEK_END);
            file_size = ftell(file);
            fseek(file, 0, SEEK_SET);
            data = (unsigned char*)std::malloc(file_size);
            fread(data, 1, file_size, file);
            fclose(file);
        }
        if (data != nullptr) {
            result->load(data, file_size);  // Pass data to load
            std::free(data);
            _assets[final_key] = std::move(asset);
        }
        return result;
    };

    void clear() {
        // Always acquire locks in a consistent order: _archive_lock then _map_lock
        std::lock_guard<std::mutex> _a_lock(_archive_lock);
        std::lock_guard<std::mutex> _m_lock(_map_lock);
        
        _assets.clear();
        if (_archive) {
            zip_close(_archive);
            _archive = nullptr;
        }
    }
};
