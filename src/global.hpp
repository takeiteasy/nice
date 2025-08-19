//
//  global.hpp
//  rpg
//
//  Created by George Watson on 01/08/2025.
//

#pragma once

#include "config.h"
#include "asset_manager.hpp"
#include "settings_manager.hpp"

int framebuffer_width(void);
int framebuffer_height(void);
void framebuffer_resize(int width, int height);

uint64_t index(int x, int y);
std::pair<int, int> unindex(uint64_t i);
