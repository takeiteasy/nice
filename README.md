# NICE

**N**ice **I**nfinite **C**hunk **E**ngine

## TODO

- Cross-chunk pathfinding
- Chunk management from lua
- Tile selection
- Arguments, settings, configs
    - Check for configs (.ini) in zip or disk
    - Expose settings to lua
- Proper save path destination 
- Audio manager
- Debug tools, chunk monitor, entity monitor, console, lua repl, assets, settings, draw call monitor, .nice maker
- Fonts, text rendering (fontstash?)
- Package binary into app, release builds
- Emscripten build
- Windows build
- Linux build
- ~~Find random empty tile function for lua~~
- ~~Rework scenes, handle in lua instead~~
- ~~Clean up lua entity functions~~
- ~~Thread pooled A*~~
- ~~Rewrite chunk serialization, must include tile.visited~~
- ~~Rework camera, integrate with ECS~~
- ~~Expose poisson to lua~~
- ~~Add sokol_input, wrap for lua~~
- ~~Fix imgui events not consuming event~~

### Ideas:

- Support fennel + teal
- Networking
- Embed zip inside binary?
- Sandboxing with lua amalgm?
- Default texture instead of crashing

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
