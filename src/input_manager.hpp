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
#include "glm/vec2.hpp"
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

#define KEY_CODES                         \
    X(INVALID, INVALID)                   \
    X(SPACE, SPACE)                       \
    X(APOSTROPHE, APOSTROPHE)             \
    X(COMMA, COMMA)                       \
    X(MINUS, MINUS)                       \
    X(PERIOD, PERIOD)                     \
    X(SLASH, SLASH)                       \
    X(0, KEY_0)                           \
    X(1, KEY_1)                           \
    X(2, KEY_2)                           \
    X(3, KEY_3)                           \
    X(4, KEY_4)                           \
    X(5, KEY_5)                           \
    X(6, KEY_6)                           \
    X(7, KEY_7)                           \
    X(8, KEY_8)                           \
    X(9, KEY_9)                           \
    X(SEMICOLON, SEMICOLON)               \
    X(EQUAL, EQUAL)                       \
    X(A, A) X(B, B) X(C, C) X(D, D)       \
    X(E, E) X(F, F) X(G, G) X(H, H)       \
    X(I, I) X(J, J) X(K, K) X(L, L)       \
    X(M, M) X(N, N) X(O, O) X(P, P)       \
    X(Q, Q) X(R, R) X(S, S) X(T, T)       \
    X(U, U) X(V, V) X(W, W) X(X, X)       \
    X(Y, Y) X(Z, Z)                       \
    X(LEFT_BRACKET, LEFT_BRACKET)         \
    X(BACKSLASH, BACKSLASH)               \
    X(RIGHT_BRACKET, RIGHT_BRACKET)       \
    X(GRAVE_ACCENT, GRAVE_ACCENT)         \
    X(WORLD_1, WORLD_1)                   \
    X(WORLD_2, WORLD_2)                   \
    X(ESCAPE, ESCAPE)                     \
    X(ENTER, ENTER)                       \
    X(TAB, TAB)                           \
    X(BACKSPACE, BACKSPACE)               \
    X(INSERT, INSERT)                     \
    X(DELETE, DELETE)                     \
    X(RIGHT, RIGHT)                       \
    X(LEFT, LEFT)                         \
    X(DOWN, DOWN)                         \
    X(UP, UP)                             \
    X(PAGE_UP, PAGE_UP)                   \
    X(PAGE_DOWN, PAGE_DOWN)               \
    X(HOME, HOME)                         \
    X(END, END)                           \
    X(CAPS_LOCK, CAPS_LOCK)               \
    X(SCROLL_LOCK, SCROLL_LOCK)           \
    X(NUM_LOCK, NUM_LOCK)                 \
    X(PRINT_SCREEN, PRINT_SCREEN)         \
    X(PAUSE, PAUSE)                       \
    X(F1, F1) X(F2, F2) X(F3, F3)         \
    X(F4, F4) X(F5, F5) X(F6, F6)         \
    X(F7, F7) X(F8, F8) X(F9, F9)         \
    X(F10, F10) X(F11, F11) X(F12, F12)   \
    X(F13, F13) X(F14, F14) X(F15, F15)   \
    X(F16, F16) X(F17, F17) X(F18, F18)   \
    X(F19, F19) X(F20, F20) X(F21, F21)   \
    X(F22, F22) X(F23, F23) X(F24, F24)   \
    X(F25, F25)                           \
    X(KP_0, KP_0) X(KP_1, KP_1)           \
    X(KP_2, KP_2) X(KP_3, KP_3)           \
    X(KP_4, KP_4) X(KP_5, KP_5)           \
    X(KP_6, KP_6) X(KP_7, KP_7)           \
    X(KP_8, KP_8) X(KP_9, KP_9)           \
    X(KP_DECIMAL, KP_DECIMAL)             \
    X(KP_DIVIDE, KP_DIVIDE)               \
    X(KP_MULTIPLY, KP_MULTIPLY)           \
    X(KP_SUBTRACT, KP_SUBTRACT)           \
    X(KP_ADD, KP_ADD)                     \
    X(KP_ENTER, KP_ENTER)                 \
    X(KP_EQUAL, KP_EQUAL)                 \
    X(LEFT_SHIFT, LEFT_SHIFT)             \
    X(LEFT_CONTROL, LEFT_CONTROL)         \
    X(LEFT_ALT, LEFT_ALT)                 \
    X(LEFT_SUPER, LEFT_SUPER)             \
    X(RIGHT_SHIFT, RIGHT_SHIFT)           \
    X(RIGHT_CONTROL, RIGHT_CONTROL)       \
    X(RIGHT_ALT, RIGHT_ALT)               \
    X(RIGHT_SUPER, RIGHT_SUPER)           \
    X(MENU, MENU)

#define MOUSE_BUTTONS                     \
    X(LEFT, LEFT)                         \
    X(RIGHT, RIGHT)                       \
    X(MIDDLE, MIDDLE)                     \
    X(INVALID, INVALID)

#define MODIFIERS                         \
    X(SHIFT, SHIFT)                       \
    X(CTRL, CTRL)                         \
    X(ALT, ALT)                           \
    X(SUPER, SUPER)                       \
    X(LMB, LMB)                           \
    X(RMB, RMB)                           \
    X(MMB, MMB)

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
#ifndef NICEPKG
    // Register of event callbacks stored as Lua registry references, keyed by event type enum
    std::unordered_map<int, int> _lua_callbacks;
    lua_State *L = nullptr;
#endif

public:
#ifndef NICEPKG
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

        // Expose key codes enum
        lua_newtable(L);
#define X(NAME, VAR) \
        lua_pushinteger(L, SAPP_KEYCODE_##NAME); \
        lua_setfield(L, -2, #VAR);
        KEY_CODES
#undef X
        lua_setglobal(L, "KeyCode");

        // Expose mouse button enum
        lua_newtable(L);
#define X(NAME, VAR) \
        lua_pushinteger(L, SAPP_MOUSEBUTTON_##NAME); \
        lua_setfield(L, -2, #VAR);
        MOUSE_BUTTONS
#undef X
        lua_setglobal(L, "MouseButton");

        // Expose modifier flags
        lua_newtable(L);
#define X(NAME, VAR) \
        lua_pushinteger(L, SAPP_MODIFIER_##NAME); \
        lua_setfield(L, -2, #VAR);
        MODIFIERS
#undef X
        lua_setglobal(L, "Modifier");

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
#endif

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

#ifndef NICEPKG
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
        if (!lua_isfunction(L, -1))
            lua_pop(L, 1); // Remove non-function from stack
        else {
            lua_pushvalue(L, -2); // Push the event table
            if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
                const char* error_msg = lua_tostring(L, -1);
                std::cout << fmt::format("Error in event callback for type {}: {}\n", static_cast<int>(event->type), error_msg);
                lua_pop(L, 1); // Remove error message
            }
        }
        lua_pop(L, 1); // Remove event table
#endif // NICEPKG
    }

#ifndef NICEPKG
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
#endif // NICEPKG

    ~InputManager() {
        // Safe destructor - callbacks should be cleaned up explicitly via cleanup_lua_callbacks()
        // If we still have callbacks, try safe cleanup
#ifndef NICEPKG
        if (!_lua_callbacks.empty())
            _lua_callbacks.clear();  // Just clear the map, don't touch Lua
#endif // NICEPKG
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
