/* https://github.com/takeiteasy/nice 

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>. */

#pragma once

#include <vector>
#include "sokol/sokol_gfx.h"
#include "sokol_gp.h"

class DummyMap {
    int _grid_width;
    int _grid_height;
    int _tile_width;
    int _tile_height;
    std::vector<bool> _grid;
    bool _ready = false;

public:
    DummyMap() { _ready = false; }
    DummyMap(int grid_width, int grid_height, int tile_width, int tile_height) {
        reset(grid_width, grid_height, tile_width, tile_height);
    }

    void reset(int grid_width=32, int grid_height=32, int tile_width=8, int tile_height=8) {
        _grid_width = grid_width;
        _grid_height = grid_height;
        _tile_width = tile_width;
        _tile_height = tile_height;
        _grid.resize(_grid_width * _grid_height, false);
        _ready = true;
    }

    void toggle_tile(int x, int y) {
        if (_ready && x >= 0 && x < _grid_width && y >= 0 && y < _grid_height)
            _grid[x * _grid_height + y] = !_grid[x * _grid_height + y];
    }

    bool check_tile(int x, int y) const {
        return !_ready ? false : x > 0 && x < _grid_width && y > 0 && y < _grid_height ? _grid[x * _grid_height + y] : false;
    }

    void draw() {
        sgp_set_color(1.f, 1.f, 1.f, 1.f);
        // Draw vertical lines
        for (int x = 0; x < _grid_width+1; x++) {
            float xx = x * _tile_width;
            sgp_draw_line(xx, 0, xx, (_tile_height*_grid_height));
        }
        // Draw horizontal lines
        for (int y = 0; y < _grid_height+1; y++) {
            float yy = y * _tile_height;
            sgp_draw_line(0, yy, (_tile_width*_grid_width), yy);
        }
        sgp_reset_color();
    }
};