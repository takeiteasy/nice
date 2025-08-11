//
// components.hh
// rpg
//
// Created by George Watson on 10/08/2025.
//

#pragma once

struct Chunk;

struct _Chunk {
    Chunk *chunk;
};

struct Position {
    int x, y;
};

struct Target {
    int x, y;
};

struct Velocity {
    float x, y;
};
