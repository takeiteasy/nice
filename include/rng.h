//
//  rng.h
//  rpg
//
//  Created by George Watson on 22/07/2025.
//

#pragma once

#include <stdint.h>
#include <float.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

void rng_srand(uint64_t seed);
uint64_t rng_rand(void);
float rng_randf(void);

void cellular_automata(unsigned int width, unsigned int height, unsigned int fill_chance, unsigned int smooth_iterations, unsigned int survive, unsigned int starve, uint8_t* dst);

void noise_fbm(unsigned int width, unsigned int height, float z, float offset_x, float offset_y, float scale, float lacunarity, float gain, int octaves, uint8_t* dst);
