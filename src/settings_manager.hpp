//
//  settings_manager.hpp
//  nice
//
//  Created by George Watson on 19/08/2025.
//

#pragma once

#include <unordered_map>
#include <string>
#include <typeinfo>
#include <stdexcept>
#include <vector>
#include <type_traits>
#include "global.hpp"

#define $Settings Settings::instance()

struct SettingBase {
    virtual ~SettingBase() = default;
    virtual const std::type_info& type_of() const = 0;
    virtual std::unique_ptr<SettingBase> clone() const = 0;
};

template<typename T>
struct Setting: public SettingBase {
    T data;
    explicit Setting(T&& value) : data(std::forward<T>(value)) {}
    explicit Setting(const T& value) : data(value) {}

    const std::type_info& type_of() const override {
        return typeid(T);
    }

    std::unique_ptr<SettingBase> clone() const override {
        return std::make_unique<Setting<T>>(data);
    }
};

class Settings: public Global<Settings> {
    std::unordered_map<std::string, std::unique_ptr<SettingBase>> _settings;
    mutable std::mutex _mutex;

public:
    Settings() = default;

    template<typename T>
    void set(const std::string& key, T&& value) {
        std::lock_guard<std::mutex> lock(_mutex);
        _settings[key] = std::make_unique<Setting<std::decay_t<T>>>(std::forward<T>(value));
    }

    template<typename T>
    const T& get(const std::string& key) const {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _settings.find(key);
        if (it == _settings.end())
            throw std::runtime_error("Setting key '" + key + "' not found");
        auto* valuePtr = dynamic_cast<Setting<T>*>(it->second.get());
        if (!valuePtr)
            throw std::runtime_error("Type mismatch for key '" + key + "'");
        return valuePtr->data;
    }

    bool has(const std::string& key) const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _settings.find(key) != _settings.end();
    }

    bool remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(_mutex);
        return _settings.erase(key) > 0;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(_mutex);
        _settings.clear();
    }

    const std::type_info& type_of(const std::string& key) const {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _settings.find(key);
        if (it == _settings.end())
            throw std::runtime_error("Setting key '" + key + "' not found");
        return it->second->type_of();
    }

    std::vector<std::string> keys() const {
        std::lock_guard<std::mutex> lock(_mutex);
        std::vector<std::string> keys;
        keys.reserve(_settings.size());
        for (const auto& pair : _settings)
            keys.push_back(pair.first);
        return keys;
    }

    // TODO: Import + export to .ini
};
