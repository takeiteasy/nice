//
//  chunk.c
//  rpg
//
//  Created by George Watson on 29/07/2025.
//

#include "chunk.h"
#include "default.glsl.h"

bool chunk_create(struct chunk *c, int x, int y, enum chunk_state state) {
    assert(c != NULL);
    c->x = x;
    c->y = y;
    c->state = state;
    memset(c->grid, 0, CHUNK_SIZE * sizeof(union tile));
    c->dirty = true;
    return pthread_mutex_init(&c->lock, NULL) == 0;
}

void chunk_destroy(struct chunk *c) {
    if (!c)
        return;
    pthread_mutex_destroy(&c->lock);
    if (sg_query_buffer_state(c->bind.vertex_buffers[0]) == SG_RESOURCESTATE_VALID)
        sg_destroy_buffer(c->bind.vertex_buffers[0]);
    memset(c, 0, sizeof(struct chunk));
}

union tile* chunk_tile(struct chunk *c, int x, int y) {
    return x < 0 || x >= CHUNK_WIDTH ||
           y < 0 || y >= CHUNK_HEIGHT ? NULL :
           &c->grid[y * CHUNK_WIDTH + x];
}

static uint8_t _calc_bitmask(uint8_t neighbours[9]) {
#define CHECK_CORNER(N, A, B) \
    neighbours[(N)] = !neighbours[(A)] || !neighbours[(B)] ? 0 : neighbours[(N)];
    CHECK_CORNER(0, 1, 3);
    CHECK_CORNER(2, 1, 5);
    CHECK_CORNER(6, 7, 3);
    CHECK_CORNER(8, 7, 5);
#undef CHECK_CORNER
    uint8_t result = 0;
    for (int y = 0, n = 0; y < 3; y++)
        for (int x = 0; x < 3; x++)
            if (!(y == 1 && x == 1))
                result += (neighbours[y * 3 + x] << n++);
    return result;
}

static uint8_t _solid(struct chunk *chunk, int x, int y, int oob) {
    return x < 0 || y < 0 || x >= CHUNK_WIDTH || y >= CHUNK_HEIGHT ? oob : chunk->grid[y * CHUNK_WIDTH + x].solid ? 1 : 0;
}

static uint8_t _bitmask(struct chunk *chunk, int cx, int cy, int oob) {
    uint8_t neighbours[9] = {0};
    for (int x = -1; x < 2; x++)
        for (int y = -1; y < 2; y++)
            neighbours[(y+1)*3+(x+1)] = !x && !y ? 0 : _solid(chunk, cx+x, cy+y, oob);
    return _calc_bitmask(neighbours);
}

void chunk_fill(struct chunk *c) {
    assert(c != NULL);

    uint8_t _grid[CHUNK_SIZE];
    cellular_automata(CHUNK_WIDTH, CHUNK_HEIGHT,
                      CHUNK_FILL_CHANCE, CHUNK_SMOOTH_ITERATIONS,
                      CHUNK_SURVIVE, CHUNK_STARVE,
                      _grid);
    chunk_each(c, _grid, ^(int x, int y, union tile *tile, void *userdata) {
        uint8_t *_grid = (uint8_t*)userdata;
        tile->solid = _grid[y * CHUNK_WIDTH + x];
        tile->bitmask = tile->solid ? _bitmask(c, x, y, 0) : 0;
    });
}

void chunk_each(struct chunk *c, void *userdata, void(^fn)(int x, int y, union tile *tile, void *userdata)) {
    assert(c != NULL && fn != NULL);
    pthread_mutex_lock(&c->lock);
    for (int y = 0; y < CHUNK_HEIGHT; y++)
        for (int x = 0; x < CHUNK_WIDTH; x++) {
            union tile *tile = &c->grid[y * CHUNK_WIDTH + x];
            fn(x, y, tile, userdata);
        }
    pthread_mutex_unlock(&c->lock);
}

static const HMM_Vec2 Autotile3x3Simplified[256] = {
    [0] = {0, 3},
    [1] = {-1,-1},
    [2] = {0, 2},
    [3] = {-1,-1},
    [4] = {-1,-1},
    [5] = {-1,-1},
    [6] = {-1,-1},
    [7] = {-1,-1},
    [8] = {3, 3},
    [9] = {-1,-1},
    [10] = {3, 2},
    [11] = {11, 3},
    [12] = {-1,-1},
    [13] = {-1,-1},
    [14] = {-1,-1},
    [15] = {-1,-1},
    [16] = {1, 3},
    [17] = {-1,-1},
    [18] = {1, 2},
    [19] = {-1,-1},
    [20] = {-1,-1},
    [21] = {-1,-1},
    [22] = {8, 3},
    [23] = {-1,-1},
    [24] = {2, 3},
    [25] = {-1,-1},
    [26] = {2, 2},
    [27] = {6, 3},
    [28] = {-1,-1},
    [29] = {-1,-1},
    [30] = {5, 3},
    [31] = {9, 3},
    [32] = {-1,-1},
    [33] = {-1,-1},
    [34] = {-1,-1},
    [35] = {-1,-1},
    [36] = {-1,-1},
    [37] = {-1,-1},
    [38] = {-1,-1},
    [39] = {-1,-1},
    [40] = {-1,-1},
    [41] = {-1,-1},
    [42] = {-1,-1},
    [43] = {-1,-1},
    [44] = {-1,-1},
    [45] = {-1,-1},
    [46] = {-1,-1},
    [47] = {-1,-1},
    [48] = {-1,-1},
    [49] = {-1,-1},
    [50] = {-1,-1},
    [51] = {-1,-1},
    [52] = {-1,-1},
    [53] = {-1,-1},
    [54] = {-1,-1},
    [55] = {-1,-1},
    [56] = {-1,-1},
    [57] = {-1,-1},
    [58] = {-1,-1},
    [59] = {-1,-1},
    [60] = {-1,-1},
    [61] = {-1,-1},
    [62] = {-1,-1},
    [63] = {-1,-1},
    [64] = {0, 0},
    [65] = {-1,-1},
    [66] = {0, 1},
    [67] = {-1,-1},
    [68] = {-1,-1},
    [69] = {-1,-1},
    [70] = {-1,-1},
    [71] = {-1,-1},
    [72] = {3, 0},
    [73] = {-1,-1},
    [74] = {3, 1},
    [75] = {7, 2},
    [76] = {-1,-1},
    [77] = {-1,-1},
    [78] = {-1,-1},
    [79] = {-1,-1},
    [80] = {1, 0},
    [81] = {-1,-1},
    [82] = {1, 1},
    [83] = {-1,-1},
    [84] = {-1,-1},
    [85] = {-1,-1},
    [86] = {4, 2},
    [87] = {-1,-1},
    [88] = {2, 0},
    [89] = {-1,-1},
    [90] = {2, 1},
    [91] = {4, 0},
    [92] = {-1,-1},
    [93] = {-1,-1},
    [94] = {7, 0},
    [95] = {10, 3},
    [96] = {-1,-1},
    [97] = {-1,-1},
    [98] = {-1,-1},
    [99] = {-1,-1},
    [100] = {-1,-1},
    [101] = {-1,-1},
    [102] = {-1,-1},
    [103] = {-1,-1},
    [104] = {11, 0},
    [105] = {-1,-1},
    [106] = {7, 1},
    [107] = {11, 2},
    [108] = {-1,-1},
    [109] = {-1,-1},
    [110] = {-1,-1},
    [111] = {-1,-1},
    [112] = {-1,-1},
    [113] = {-1,-1},
    [114] = {-1,-1},
    [115] = {-1,-1},
    [116] = {-1,-1},
    [117] = {-1,-1},
    [118] = {-1,-1},
    [119] = {-1,-1},
    [120] = {6, 0},
    [121] = {-1,-1},
    [122] = {4, 3},
    [123] = {11, 1},
    [124] = {-1,-1},
    [125] = {-1,-1},
    [126] = {9, 1},
    [127] = {6, 2},
    [128] = {-1,-1},
    [129] = {-1,-1},
    [130] = {-1,-1},
    [131] = {-1,-1},
    [132] = {-1,-1},
    [133] = {-1,-1},
    [134] = {-1,-1},
    [135] = {-1,-1},
    [136] = {-1,-1},
    [137] = {-1,-1},
    [138] = {-1,-1},
    [139] = {-1,-1},
    [140] = {-1,-1},
    [141] = {-1,-1},
    [142] = {-1,-1},
    [143] = {-1,-1},
    [144] = {-1,-1},
    [145] = {-1,-1},
    [146] = {-1,-1},
    [147] = {-1,-1},
    [148] = {-1,-1},
    [149] = {-1,-1},
    [150] = {-1,-1},
    [151] = {-1,-1},
    [152] = {-1,-1},
    [153] = {-1,-1},
    [154] = {-1,-1},
    [155] = {-1,-1},
    [156] = {-1,-1},
    [157] = {-1,-1},
    [158] = {-1,-1},
    [159] = {-1,-1},
    [160] = {-1,-1},
    [161] = {-1,-1},
    [162] = {-1,-1},
    [163] = {-1,-1},
    [164] = {-1,-1},
    [165] = {-1,-1},
    [166] = {-1,-1},
    [167] = {-1,-1},
    [168] = {-1,-1},
    [169] = {-1,-1},
    [170] = {-1,-1},
    [171] = {-1,-1},
    [172] = {-1,-1},
    [173] = {-1,-1},
    [174] = {-1,-1},
    [175] = {-1,-1},
    [176] = {-1,-1},
    [177] = {-1,-1},
    [178] = {-1,-1},
    [179] = {-1,-1},
    [180] = {-1,-1},
    [181] = {-1,-1},
    [182] = {-1,-1},
    [183] = {-1,-1},
    [184] = {-1,-1},
    [185] = {-1,-1},
    [186] = {-1,-1},
    [187] = {-1,-1},
    [188] = {-1,-1},
    [189] = {-1,-1},
    [190] = {-1,-1},
    [191] = {-1,-1},
    [192] = {-1,-1},
    [193] = {-1,-1},
    [194] = {-1,-1},
    [195] = {-1,-1},
    [196] = {-1,-1},
    [197] = {-1,-1},
    [198] = {-1,-1},
    [199] = {-1,-1},
    [200] = {-1,-1},
    [201] = {-1,-1},
    [202] = {-1,-1},
    [203] = {-1,-1},
    [204] = {-1,-1},
    [205] = {-1,-1},
    [206] = {-1,-1},
    [207] = {-1,-1},
    [208] = {8, 0},
    [209] = {-1,-1},
    [210] = {4, 1},
    [211] = {-1,-1},
    [212] = {-1,-1},
    [213] = {-1,-1},
    [214] = {8, 1},
    [215] = {-1,-1},
    [216] = {5, 0},
    [217] = {-1,-1},
    [218] = {7, 3},
    [219] = {10, 2},
    [220] = {-1,-1},
    [221] = {-1,-1},
    [222] = {8, 2},
    [223] = {5, 2},
    [224] = {-1,-1},
    [225] = {-1,-1},
    [226] = {-1,-1},
    [227] = {-1,-1},
    [228] = {-1,-1},
    [229] = {-1,-1},
    [230] = {-1,-1},
    [231] = {-1,-1},
    [232] = {-1,-1},
    [233] = {-1,-1},
    [234] = {-1,-1},
    [235] = {-1,-1},
    [236] = {-1,-1},
    [237] = {-1,-1},
    [238] = {-1,-1},
    [239] = {-1,-1},
    [240] = {-1,-1},
    [241] = {-1,-1},
    [242] = {-1,-1},
    [243] = {-1,-1},
    [244] = {-1,-1},
    [245] = {-1,-1},
    [246] = {-1,-1},
    [247] = {-1,-1},
    [248] = {10, 0},
    [249] = {-1,-1},
    [250] = {9, 0},
    [251] = {6, 1},
    [252] = {-1,-1},
    [253] = {-1,-1},
    [254] = {5, 1},
    [255] = {9, 2},
};

void chunk_draw(struct chunk *c, struct texture *texture, struct camera *camera) {
    assert(c != NULL && camera != NULL);
    if (texture) {
        c->bind.images[IMG_tex] = texture->image;
        c->bind.samplers[SMP_smp] = texture->sampler;
    }

    if (c->dirty) {
        if (sg_query_buffer_state(c->bind.vertex_buffers[0]) == SG_RESOURCESTATE_VALID)
            sg_destroy_buffer(c->bind.vertex_buffers[0]);
        c->dirty = false;

        float hw = sapp_width() / 2.f;
        float hh = sapp_height() / 2.f;
        size_t mem_size = CHUNK_SIZE * 6 * sizeof(struct chunk_vertex);
        struct chunk_vertex *vertices = malloc(mem_size);
        memset(vertices, 0, mem_size);
        for (int x = 0; x < CHUNK_WIDTH; x++)
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                union tile *tile = &c->grid[y * CHUNK_WIDTH + x];
                if (!tile->solid)
                    continue;
                tile->bitmask = tile->solid ? _bitmask(c, x, y, 0) : 0;

                HMM_Vec2 clip = Autotile3x3Simplified[tile->bitmask];
                struct rect src = {
                    (clip.X * TILE_WIDTH) + ((clip.X + 1) * TILE_PADDING),
                    (clip.Y * TILE_HEIGHT) + ((clip.Y + 1) * TILE_PADDING),
                    TILE_WIDTH, TILE_HEIGHT
                };

                float hqw = TILE_WIDTH / 2.f;
                float hqh = TILE_HEIGHT / 2.f;
                HMM_Vec2 _positions[] = {
                    {hw - hqw, hh - hqh}, // Top-left
                    {hw + hqw, hh - hqh}, // Top-right
                    {hw + hqw, hh + hqh}, // Bottom-right
                    {hw - hqw, hh + hqh}, // Bottom-left
                };

                float iw = 1.f/texture->width;
                float ih = 1.f/texture->height;
                float tl = src.x * iw;
                float tt = src.y * ih;
                float tr = (src.x + src.w) * iw;
                float tb = (src.y + src.h) * ih;
                HMM_Vec2 _texcoords[4] = {
                    {tl, tt}, // top left
                    {tr, tt}, // top right
                    {tr, tb}, // bottom right
                    {tl, tb}, // bottom left
                };

                uint16_t indices[] = { 0, 1, 2, 2, 3, 0 };
                for (int i = 0; i < 6; i++) {
                    struct chunk_vertex *v = &vertices[(y * CHUNK_WIDTH + x) * 6 + i];
                    HMM_Vec2 offset = HMM_V2(x * TILE_WIDTH, y * TILE_HEIGHT);
                    v->position = HMM_AddV2(_positions[indices[i]], offset);
                    v->texcoord = _texcoords[indices[i]];
                    v->color = HMM_V4(1.f, 1.f, 1.f, 1.f);
                }
            }

        c->bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
            .data = {
                .ptr = vertices,
                .size = mem_size,
            },
        });
        free(vertices);
    }

    if (camera->dirty)
        c->mvp = HMM_MulM4(camera_mvp(camera, sapp_width(), sapp_height()),
                           HMM_Translate(HMM_V3(c->x * CHUNK_WIDTH * TILE_WIDTH,
                                                c->y * CHUNK_HEIGHT * TILE_HEIGHT,
                                                0)));

    sg_apply_bindings(&c->bind);
    vs_params_t params;
    memcpy(params.mvp, &c->mvp.Elements, sizeof(float) * 16);
    sg_apply_uniforms(UB_vs_params, &SG_RANGE(params));
    sg_draw(0, CHUNK_SIZE * 6, 1);
}
