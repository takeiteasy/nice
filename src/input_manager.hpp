//
//  input_manager.hpp
//  nice
//
//  Created by George Watson on 03/09/2025.
//

#pragma once

#include "global.hpp"
#include "sokol/sokol_app.h"
#include "sokol/sokol_time.h"
#include "minilua.h"
#include "fmt/format.h"
#include <iostream>

#define $Input InputManager::instance()

#define EVENT_TYPES                       \
    X(KEY_DOWN, key_down)                 \
    X(KEY_UP, key_up)                     \
    X(MOUSE_DOWN, mouse_down)             \
    X(MOUSE_UP, mouse_up)                 \
    X(MOUSE_SCROLL, mouse_scroll)         \
    X(MOUSE_MOVE, mouse_move)             \
    X(MOUSE_ENTER, mouse_enter)           \
    X(MOUSE_LEAVE, mouse_leave)           \
    X(RESIZED, resized)                   \
    X(ICONIFIED, iconified)               \
    X(RESTORED, restored)                 \
    X(FOCUSED, focused)                   \
    X(UNFOCUSED, unfocused)               \
    X(SUSPENDED, suspended)               \
    X(RESUMED, resumed)                   \
    X(QUIT_REQUESTED, quit_requested)     \
    X(CLIPBOARD_PASTED, clipboard_pasted) \
    X(FILES_DROPPED, files_dropped)

class InputManager: public Global<InputManager> {
    struct InputState {
        bool _keyboard_state[349] = {false};
        bool _mouse_buttons[3] = {false};
        uint64_t _keyboard_last_time[349] = {0};
        uint64_t _mouse_last_time[3] = {0};
        uint32_t _modifiers = 0;
        glm::vec2 _mouse_position = glm::vec2(0.f);
        glm::vec2 _mouse_wheel = glm::vec2(0.f);
    };
    bool _window_is_iconified = false;
    bool _window_is_focused = true;
    bool _window_is_suspended = false;
    glm::vec2 _window_size = glm::vec2(sapp_widthf(), sapp_heightf());
    InputState _input_state, _input_state_prev;
    // Register of event callbacks stored as Lua registry references, keyed by event type enum
    std::unordered_map<int, int> _lua_callbacks;
    lua_State *L = nullptr;

public:
    void load_into_lua(lua_State *_L) {
        L = _L;
        
        lua_register(L, "is_key_down", [](lua_State *L) -> int {
            int key = static_cast<int>(luaL_checkinteger(L, 1));
            if (key < 0 || key >= 349) {
                lua_pushboolean(L, false);
                return 1;
            }
            lua_pushboolean(L, $Input.is_down(static_cast<sapp_keycode>(key)));
            return 1;
        });

        lua_register(L, "is_key_pressed", [](lua_State *L) -> int {
            int key = static_cast<int>(luaL_checkinteger(L, 1));
            if (key < 0 || key >= 349) {
                lua_pushboolean(L, false);
                return 1;
            }
            lua_pushboolean(L, $Input.is_pressed(static_cast<sapp_keycode>(key)));
            return 1;
        });

        lua_register(L, "is_key_released", [](lua_State *L) -> int {
            int key = static_cast<int>(luaL_checkinteger(L, 1));
            if (key < 0 || key >= 349) {
                lua_pushboolean(L, false);
                return 1;
            }
            lua_pushboolean(L, $Input.is_released(static_cast<sapp_keycode>(key)));
            return 1;
        });

        lua_register(L, "is_mouse_button_down", [](lua_State *L) -> int {
            int button = static_cast<int>(luaL_checkinteger(L, 1));
            if (button < 0 || button >= 3) {
                lua_pushboolean(L, false);
                return 1;
            }
            lua_pushboolean(L, $Input.is_down(static_cast<sapp_mousebutton>(button)));
            return 1;
        });

        lua_register(L, "is_mouse_button_pressed", [](lua_State *L) -> int {
            int button = static_cast<int>(luaL_checkinteger(L, 1));
            if (button < 0 || button >= 3) {
                lua_pushboolean(L, false);
                return 1;
            }
            lua_pushboolean(L, $Input.is_pressed(static_cast<sapp_mousebutton>(button)));
            return 1;
        });

        lua_register(L, "is_mouse_button_released", [](lua_State *L) -> int {
            int button = static_cast<int>(luaL_checkinteger(L, 1));
            if (button < 0 || button >= 3) {
                lua_pushboolean(L, false);
                return 1;
            }
            lua_pushboolean(L, $Input.is_released(static_cast<sapp_mousebutton>(button)));
            return 1;
        });

        lua_register(L, "mouse_position", [](lua_State *L) -> int {
            glm::vec2 pos = $Input.mouse_position();
            lua_pushnumber(L, pos.x);
            lua_pushnumber(L, pos.y);
            return 2;
        });

        lua_register(L, "mouse_wheel", [](lua_State *L) -> int {
            glm::vec2 wheel = $Input.mouse_wheel();
            lua_pushnumber(L, wheel.x);
            lua_pushnumber(L, wheel.y);
            return 2;
        });

        lua_register(L, "mouse_delta", [](lua_State *L) -> int {
            glm::vec2 delta = $Input.mouse_delta();
            lua_pushnumber(L, delta.x);
            lua_pushnumber(L, delta.y);
            return 2;
        });

        lua_register(L, "mouse_wheel_delta", [](lua_State *L) -> int {
            glm::vec2 delta = $Input.mouse_wheel_delta();
            lua_pushnumber(L, delta.x);
            lua_pushnumber(L, delta.y);
            return 2;
        });

        lua_register(L, "window_size", [](lua_State *L) -> int {
            glm::vec2 size = $Input.window_size();
            lua_pushnumber(L, size.x);
            lua_pushnumber(L, size.y);
            return 2;
        });

        lua_register(L, "window_is_iconified", [](lua_State *L) -> int {
            lua_pushboolean(L, $Input.window_is_iconified());
            return 1;
        });

        lua_register(L, "window_is_focused", [](lua_State *L) -> int {
            lua_pushboolean(L, $Input.window_is_focused());
            return 1;
        });

        lua_register(L, "window_is_suspended", [](lua_State *L) -> int {
            lua_pushboolean(L, $Input.window_is_suspended());
            return 1;
        });

        lua_newtable(L);
#define X(NAME, VAR) \
        lua_pushinteger(L, SAPP_EVENTTYPE_##NAME); \
        lua_setfield(L, -2, #VAR);
        EVENT_TYPES
#undef X
        lua_setglobal(L, "EventType");

        // Register callback management functions
        lua_register(L, "register_event_callback", [](lua_State *L) -> int {
            if (!lua_isinteger(L, 1) || !lua_isfunction(L, 2)) {
                luaL_error(L, "register_event_callback expects (integer, function)");
                return 0;
            }
            
            int event_type = static_cast<int>(luaL_checkinteger(L, 1));
            
            // Store the function in the registry and get a reference
            lua_pushvalue(L, 2); // Duplicate the function
            int ref = luaL_ref(L, LUA_REGISTRYINDEX);
            
            // Clear any existing callback for this event
            auto& callbacks = $Input._lua_callbacks;
            auto it = callbacks.find(event_type);
            if (it != callbacks.end())
                luaL_unref(L, LUA_REGISTRYINDEX, it->second);
            
            callbacks[event_type] = ref;
            return 0;
        });

        lua_register(L, "unregister_event_callback", [](lua_State *L) -> int {
            if (!lua_isinteger(L, 1)) {
                luaL_error(L, "unregister_event_callback expects (integer)");
                return 0;
            }
            
            int event_type = static_cast<int>(luaL_checkinteger(L, 1));
            auto& callbacks = $Input._lua_callbacks;
            auto it = callbacks.find(event_type);
            if (it != callbacks.end()) {
                luaL_unref(L, LUA_REGISTRYINDEX, it->second);
                callbacks.erase(it);
            }
            return 0;
        });
    }

    void handle(const sapp_event* event) {
        switch (event->type) {
            case SAPP_EVENTTYPE_KEY_DOWN:
            case SAPP_EVENTTYPE_KEY_UP:
                if (event->key_code <= SAPP_KEYCODE_MENU) {
                    _input_state._keyboard_state[event->key_code] = (event->type == SAPP_EVENTTYPE_KEY_DOWN);
                    _input_state._keyboard_last_time[event->key_code] = stm_now();
                }
                _input_state._modifiers = event->modifiers;
                break;
            case SAPP_EVENTTYPE_MOUSE_DOWN:
            case SAPP_EVENTTYPE_MOUSE_UP:
                if (event->mouse_button >= SAPP_MOUSEBUTTON_LEFT && event->mouse_button <= SAPP_MOUSEBUTTON_RIGHT)
                    _input_state._mouse_buttons[event->mouse_button] = (event->type == SAPP_EVENTTYPE_MOUSE_DOWN);
                _input_state._modifiers = event->modifiers;
                break;
            case SAPP_EVENTTYPE_MOUSE_SCROLL:
                _input_state._mouse_wheel = glm::vec2(event->scroll_x, event->scroll_y);
                _input_state._modifiers = event->modifiers;
                break;
            case SAPP_EVENTTYPE_MOUSE_MOVE:
                _input_state._mouse_position = glm::vec2(event->mouse_x, event->mouse_y);
                _input_state._modifiers = event->modifiers;
                break;
            case SAPP_EVENTTYPE_MOUSE_ENTER:
            case SAPP_EVENTTYPE_MOUSE_LEAVE:
                _input_state._mouse_wheel = glm::vec2(0.f);
                _input_state._modifiers = event->modifiers;
                break;
            default:
                return;
        }

        lua_newtable(L);
        switch (event->type) {
            case SAPP_EVENTTYPE_KEY_DOWN:
            case SAPP_EVENTTYPE_KEY_UP:
                lua_pushstring(L, "type");
                lua_pushstring(L, event->type == SAPP_EVENTTYPE_KEY_DOWN ? "KEY_DOWN" : "KEY_UP");
                lua_settable(L, -3);

                lua_pushstring(L, "key_code");
                lua_pushinteger(L, event->key_code);
                lua_settable(L, -3);

                lua_pushstring(L, "char");
                if (event->char_code != 0)
                    lua_pushinteger(L, event->char_code);
                else
                    lua_pushnil(L);
                lua_settable(L, -3);

                lua_pushstring(L, "modifiers");
                lua_pushinteger(L, event->modifiers);
                lua_settable(L, -3);
                break;
            case SAPP_EVENTTYPE_MOUSE_DOWN:
            case SAPP_EVENTTYPE_MOUSE_UP:
                lua_pushstring(L, "type");
                lua_pushstring(L, event->type == SAPP_EVENTTYPE_MOUSE_DOWN ? "MOUSE_DOWN" : "MOUSE_UP");
                lua_settable(L, -3);

                lua_pushstring(L, "button");
                lua_pushinteger(L, event->mouse_button);
                lua_settable(L, -3);

                lua_pushstring(L, "modifiers");
                lua_pushinteger(L, event->modifiers);
                lua_settable(L, -3);
                break;
            case SAPP_EVENTTYPE_MOUSE_SCROLL:
                lua_pushstring(L, "type");
                lua_pushstring(L, "MOUSE_SCROLL");
                lua_settable(L, -3);

                lua_pushstring(L, "scroll");
                lua_newtable(L);
                lua_pushinteger(L, event->scroll_x);
                lua_setfield(L, -2, "x");
                lua_pushinteger(L, event->scroll_y);
                lua_setfield(L, -2, "y");
                lua_settable(L, -3);
                break;
            case SAPP_EVENTTYPE_MOUSE_MOVE:
                lua_pushstring(L, "type");
                lua_pushstring(L, "MOUSE_MOVE");
                lua_settable(L, -3);

                lua_pushstring(L, "position");
                lua_newtable(L);
                lua_pushinteger(L, event->mouse_x);
                lua_setfield(L, -2, "x");
                lua_pushinteger(L, event->mouse_y);
                lua_setfield(L, -2, "y");
                lua_pushinteger(L, event->mouse_dx);
                lua_setfield(L, -2, "dx");
                lua_pushinteger(L, event->mouse_dy);
                lua_setfield(L, -2, "dy");
                lua_settable(L, -3);
                break;
            case SAPP_EVENTTYPE_MOUSE_ENTER:
            case SAPP_EVENTTYPE_MOUSE_LEAVE:
                lua_pushstring(L, "type");
                lua_pushstring(L, event->type == SAPP_EVENTTYPE_MOUSE_ENTER ? "MOUSE_ENTER" : "MOUSE_LEAVE");
                lua_settable(L, -3);

                lua_pushstring(L, "position");
                lua_newtable(L);
                lua_pushinteger(L, event->mouse_x);
                lua_setfield(L, -2, "x");
                lua_pushinteger(L, event->mouse_y);
                lua_setfield(L, -2, "y");
                lua_settable(L, -3);
                break;
            case SAPP_EVENTTYPE_RESIZED:
                lua_pushstring(L, "type");
                lua_pushstring(L, "RESIZED");
                lua_settable(L, -3);

                lua_pushstring(L, "size");
                lua_newtable(L);
                lua_pushinteger(L, sapp_width());
                lua_setfield(L, -2, "width");
                lua_pushinteger(L, sapp_height());
                lua_setfield(L, -2, "height");
                lua_settable(L, -3);
                break;
            case SAPP_EVENTTYPE_ICONIFIED:
                lua_pushstring(L, "type");
                lua_pushstring(L, "ICONIFIED");
                lua_settable(L, -3);
                break;
            case SAPP_EVENTTYPE_RESTORED:
                lua_pushstring(L, "type");
                lua_pushstring(L, "RESTORED");
                lua_settable(L, -3);
                break;
            case SAPP_EVENTTYPE_FOCUSED:
                lua_pushstring(L, "type");
                lua_pushstring(L, "FOCUSED");
                lua_settable(L, -3);
                break;
            case SAPP_EVENTTYPE_UNFOCUSED:
                lua_pushstring(L, "type");
                lua_pushstring(L, "UNFOCUSED");
                lua_settable(L, -3);
                break;
            case SAPP_EVENTTYPE_SUSPENDED:
                lua_pushstring(L, "type");
                lua_pushstring(L, "SUSPENDED");
                lua_settable(L, -3);
                break;
            case SAPP_EVENTTYPE_RESUMED:
                lua_pushstring(L, "type");
                lua_pushstring(L, "RESUMED");
                lua_settable(L, -3);
                break;
            case SAPP_EVENTTYPE_QUIT_REQUESTED:
                lua_pushstring(L, "type");
                lua_pushstring(L, "QUIT_REQUESTED");
                lua_settable(L, -3);
                break;
            case SAPP_EVENTTYPE_CLIPBOARD_PASTED:
                lua_pushstring(L, "type");
                lua_pushstring(L, "CLIPBOARD_PASTED");
                lua_settable(L, -3);
                lua_pushstring(L, "clipboard");
                lua_pushstring(L, sapp_get_clipboard_string());
                lua_settable(L, -3);
                break;
            case SAPP_EVENTTYPE_FILES_DROPPED:
                lua_pushstring(L, "type");
                lua_pushstring(L, "FILES_DROPPED");
                lua_settable(L, -3);
                lua_pushstring(L, "count");
                lua_pushinteger(L, sapp_get_num_dropped_files());

                lua_newtable(L);
                for (int i = 0; i < sapp_get_num_dropped_files(); i++) {
                    lua_pushinteger(L, i);
                    lua_pushstring(L, sapp_get_dropped_file_path(i));
                    lua_settable(L, -3);
                }
                lua_settable(L, -3);
                break;
            default:
                return;
        }
        lua_pushstring(L, "modifiers");
        lua_pushinteger(L, event->modifiers);
        lua_settable(L, -3);

        // Look for registered callback using the event type enum value
        auto it = _lua_callbacks.find(static_cast<int>(event->type));
        if (it == _lua_callbacks.end()) {
            lua_pop(L, 1); // Remove event table
            return; // No callback registered for this event type
        }
        
        lua_rawgeti(L, LUA_REGISTRYINDEX, it->second); // Get the callback function
        if (!lua_isfunction(L, -1)) {
            lua_pop(L, 1); // Remove non-function from stack
        } else {
            lua_pushvalue(L, -2); // Push the event table
            if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
                const char* error_msg = lua_tostring(L, -1);
                std::cout << fmt::format("Error in event callback for type {}: {}\n", static_cast<int>(event->type), error_msg);
                lua_pop(L, 1); // Remove error message
            }
        }
        lua_pop(L, 1); // Remove event table
    }

    void cleanup_lua_callbacks() {
        // Explicit cleanup function to call before Lua state is destroyed
        if (L != nullptr) {
            for (auto& [event_type, ref] : _lua_callbacks)
                if (ref != LUA_NOREF && ref != LUA_REFNIL)
                    luaL_unref(L, LUA_REGISTRYINDEX, ref);
            _lua_callbacks.clear();
            L = nullptr;  // Clear the pointer to prevent destructor issues
        }
    }

    ~InputManager() {
        // Safe destructor - callbacks should be cleaned up explicitly via cleanup_lua_callbacks()
        // If we still have callbacks, try safe cleanup
        if (!_lua_callbacks.empty())
            _lua_callbacks.clear();  // Just clear the map, don't touch Lua
    }

    void update() {
        _input_state_prev = _input_state;
        // Preserve mouse position between frames, only reset button/key states
        glm::vec2 preserved_mouse_pos = _input_state._mouse_position;
        _input_state = InputState();
        _input_state._mouse_position = preserved_mouse_pos;
    }

    template<typename T>
    bool is_down(T key) {
        if constexpr (std::is_same_v<T, sapp_keycode>) {
            return _input_state._keyboard_state[static_cast<int>(key)];
        } else if constexpr (std::is_same_v<T, sapp_mousebutton>) {
            return _input_state._mouse_buttons[static_cast<int>(key)];
        }
    }

    template<typename T>
    bool is_pressed(T key) const {
        if constexpr (std::is_same_v<T, sapp_keycode>) {
            return _input_state._keyboard_state[static_cast<int>(key)];
        } else if constexpr (std::is_same_v<T, sapp_mousebutton>) {
            return _input_state._mouse_buttons[static_cast<int>(key)];
        }
    }

    template<typename T>
    bool is_released(T key) const {
        if constexpr (std::is_same_v<T, sapp_keycode>) {
            return !_input_state._keyboard_state[static_cast<int>(key)] && _input_state_prev._keyboard_state[static_cast<int>(key)];
        } else if constexpr (std::is_same_v<T, sapp_mousebutton>) {
            return !_input_state._mouse_buttons[static_cast<int>(key)] && _input_state_prev._mouse_buttons[static_cast<int>(key)];
        }
    }

    template<typename T>
    uint64_t ms_since_last_change(T key) const {
        if constexpr (std::is_same_v<T, sapp_keycode>) {
            return stm_ms(stm_diff(stm_now(), _input_state._keyboard_last_time[static_cast<int>(key)]));
        } else if constexpr (std::is_same_v<T, sapp_mousebutton>) {
            return stm_ms(stm_diff(stm_now(), _input_state._mouse_last_time[static_cast<int>(key)]));
        }
    }

    glm::vec2 mouse_position() const {
        return _input_state._mouse_position;
    }

    glm::vec2 mouse_wheel() const {
        return _input_state._mouse_wheel;
    }

    glm::vec2 mouse_delta() const {
        return _input_state._mouse_position - _input_state_prev._mouse_position;
    }

    glm::vec2 mouse_wheel_delta() const {
        return _input_state._mouse_wheel - _input_state_prev._mouse_wheel;
    }

    bool is_shift_down() const {
        return _input_state._modifiers & SAPP_MODIFIER_SHIFT;
    }

    bool is_control_down() const {
        return _input_state._modifiers & SAPP_MODIFIER_CTRL;
    }

    bool is_alt_down() const {
        return _input_state._modifiers & SAPP_MODIFIER_ALT;
    }

    bool is_super_down() const {
        return _input_state._modifiers & SAPP_MODIFIER_SUPER;
    }

    bool is_any_modifier_down() const {
        return _input_state._modifiers != 0;
    }

    glm::vec2 window_size() const {
        return _window_size;
    }

    bool window_is_iconified() const {
        return _window_is_iconified;
    }

    bool window_is_focused() const {
        return _window_is_focused;
    }

    bool window_is_suspended() const {
        return _window_is_suspended;
    }
};
