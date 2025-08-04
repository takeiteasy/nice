//
//  scene.h
//  rpg
//
//  Created by George Watson on 22/07/2025.
//

#pragma once

#include "config.h"

struct scene {
    const char *name;
    void(*enter)(void);
    void(*exit)(void);
    void(*step)(void);
};

#define X(NAME) extern struct scene NAME##_scene;
SCENES
#undef X

void set_scene(struct scene *scene);
void set_scene_named(const char *name);
