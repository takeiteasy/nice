//
//  global.hpp
//  nice
//
//  Created by George Watson on 28/08/2025.
//

#pragma once

#include <memory>
#include <mutex>
#include <utility>

template<typename T>
class Global {
    inline static std::unique_ptr<T> _instance = nullptr;
    inline static std::once_flag _once;

protected:
    Global() = default;

private:
    Global(const Global<T>&) = delete;
    Global& operator=(const Global<T>&) = delete;

public:
    static T& instance() {
        std::call_once(_once, []() {
            _instance = std::unique_ptr<T>(new T());
        });
        return *_instance;
    }
};
