//
//  ice.hpp
//  ice
//
//  Created by George Watson on 01/08/2025.
//

#pragma once

#include "ice_config.h"
#include "asset_manager.hpp"
#include "settings_manager.hpp"

struct sapp_event;

struct scene {
    const char *name;
    void(*enter)(void);
    void(*exit)(void);
    void(*event)(const sapp_event *event);
    void(*step)(void);
};

#define X(NAME) extern struct scene NAME##_scene;
SCENES
#undef X

void set_scene(struct scene *scene);
void set_scene_named(const char *name);

int framebuffer_width(void);
int framebuffer_height(void);
void framebuffer_resize(int width, int height);

uint64_t index(int x, int y);
std::pair<int, int> unindex(uint64_t i);
