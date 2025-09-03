# NICE

**N**ice **I**nfinite **C**hunk **E**ngine

## TODO

- [ ] Rewrite chunk serialization, must include tile.visited
- [ ] Chunk queries from lua for things like hit detection and collision
- [ ] Collision for entities with walls and each other, lua callback on collision
- [ ] Tile destruction and chunk modification from lua 
- [ ] Cursor + tile selection
- [ ] Thread pooled A*
- [ ] Arguments, settings, configs
    - [ ] Handle arguments from lua?
    - [ ] Check for configs in zip
    - [ ] Configs in lua instead of JSON? Both?
    - [ ] Expose settings to lua
- [ ] Default texture instead of crashing
- [ ] Proper save path destination 
- [ ] Audio manager
- [ ] Menu scene (minimal)
- [ ] Scene transition, loading screen
- [ ] Debug tools, chunk monitor, entity monitor, console, modify lua file, lua repl, assets, settings, draw call monitor
- [ ] Fonts, text rendering
- [ ] Package binary into app, release builds for Make
- [ ] Emscripten build
- [ ] Windows build
- [ ] Linux build
- [x] ~~Rework camera, integrate with ECS~~
- [x] ~~Expose poisson to lua~~
- [X] ~~Add sokol_input, wrap for lua~~
- [X] ~~Fix imgui events not consuming event~~

### Ideas:

- [ ] Rework scenes, handle in lua instead
    - [ ] Maybe lua scenes could be used in tandem with cpp scenes?
- [ ] Support fennel + teal
- [ ] Optional autotile, different autotile configuration, autotile tool
- [ ] Isometric rendering
- [ ] Built in editor, generate lua + run dynamically (play-nice)
- [ ] Networking built in, sync world+ECS in background automatically
- [ ] Embed zip inside binary
- [ ] Cellular automata optional, override in lua or add alternative methods 
- [ ] Chunk loading rules
- [ ] Sandboxing with lua amalgm?

## LICENSE
```
Nice Infinite Chunk Engine

Copyright (C) 2025 George Watson

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```
