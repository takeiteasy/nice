/* table.h -- https://github.com/takeiteasy/table.h

 Copyright (C) 2024  George Watson

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

 This library is based off https://github.com/billziss-gh/imap/
 Copyright (c) 2023 Bill Zissimopoulos. All rights reserved.

 imap -- https://github.com/billziss-gh/imap

 MIT License

 Copyright (c) 2023 Bill Zissimopoulos

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE. */

#ifndef TABLE_HEADER
#define TABLE_HEADER
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __clang__
#define _COMPILER_CLANG
#if __clang_major__ < 15
#error clang version must be >= 15 to use typeof
#endif
#elif defined(__GNUC__)
#if __GNUC__ < 10 || (__GNUC__ < 10 && __GNUC_MINOR__ < 4)
#error gcc version must be >= 10.4 to use typeof
#endif
#else
#error unsupported compiler, please use clang or gcc
#endif

#ifndef typeof
#define typeof(X) __typeof__((X))
#endif

#ifndef __has_feature
#define __has_feature(x) 0
#endif
#ifndef __has_extension
#define __has_extension __has_feature
#endif

#if !__has_extension(blocks)
#error your compiler does not support blocks, please use a later version of clang or gcc
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifndef TABLE_MALLOC
#define TABLE_MALLOC malloc
#endif
#ifndef TABLE_FREE
#define TABLE_FREE free
#endif

#ifndef TABLE_INITIAL_CAPACITY
#define TABLE_INITIAL_CAPACITY 8
#endif

typedef struct imap_node_t imap_node_t;

typedef struct imap_t {
    imap_node_t *tree;
    size_t count, capacity;
} imap_t;

typedef uint64_t(*table_hash_fn)(const void *data, size_t len, uint32_t seed);

typedef enum entry_type {
    ENTRY_INT = 0,
    ENTRY_STR,
    ENTRY_PTR
} table_entry_type;

typedef struct table_entry {
    uint64_t value;
    table_entry_type type;
} table_entry_t;

typedef struct table {
    imap_t map, keys;
    table_hash_fn hashfn;
    uint64_t seed;
} table_t;

#define _T_TYPE(T)                          \
    _Generic((T),                           \
        char: ENTRY_INT,                    \
        short: ENTRY_INT,                   \
        int: ENTRY_INT,                     \
        long: ENTRY_INT,                    \
        long long: ENTRY_INT,               \
        unsigned char: ENTRY_INT,           \
        unsigned short: ENTRY_INT,          \
        unsigned int: ENTRY_INT,            \
        unsigned long: ENTRY_INT,           \
        unsigned long long: ENTRY_INT,      \
        char *: ENTRY_STR,                  \
        const char *: ENTRY_STR,            \
        unsigned char *: ENTRY_STR,         \
        const unsigned char *: ENTRY_STR,   \
        void *: ENTRY_PTR,                  \
        default: ENTRY_PTR)

#define _T_COERCE(T, V)                         \
    _Generic((int (*)[_T_TYPE(V)])NULL,         \
        int (*)[ENTRY_INT]: _table_int_to_int,  \
        int (*)[ENTRY_STR]: _table_str_to_int,  \
        default: _table_void_to_int)((T), (V))

#define _T_IMAP(C)                      \
    (imap_t)                            \
    {                                   \
        .capacity = (C),                \
        .count = 0,                     \
        .tree = _imap_ensure(NULL, (C)) \
    }
#define _T_TABLE(FN, C, S)      \
    (table_t)                   \
    {                           \
        .hashfn = (FN),         \
        .seed = (S),            \
        .map = _T_IMAP((C)),    \
        .keys = _T_IMAP((C)),   \
    }
#define _TABLE(FN, C, S) \
    _T_TABLE(FN, ((C) > TABLE_INITIAL_CAPACITY ? (C) : TABLE_INITIAL_CAPACITY), S)
#define table() \
    (_TABLE(_table_murmur, TABLE_INITIAL_CAPACITY, 0))

void table_free(table_t *table);

#define table_set(T, A, B)                                  \
    _Generic((int (*)[_T_TYPE(A)][_T_TYPE(B)])NULL,         \
        int(*)[ENTRY_INT][ENTRY_INT]: _table_set_int,       \
        int(*)[ENTRY_INT][ENTRY_STR]: _table_set_int,       \
        int(*)[ENTRY_INT][ENTRY_PTR]: _table_set_int,       \
        int(*)[ENTRY_STR][ENTRY_INT]: _table_set_str,       \
        int(*)[ENTRY_STR][ENTRY_STR]: _table_set_str,       \
        int(*)[ENTRY_STR][ENTRY_PTR]: _table_set_str,       \
        int(*)[ENTRY_PTR][ENTRY_INT]: _table_set_void,      \
        int(*)[ENTRY_PTR][ENTRY_STR]: _table_set_void,      \
        int(*)[ENTRY_PTR][ENTRY_PTR]: _table_set_void)((T), (A), _T_COERCE((T), (B)))

#define _T_GET(T, K)                        \
    _Generic((int (*)[_T_TYPE(K)])NULL,     \
        int(*)[ENTRY_INT]: _table_get_int,  \
        int(*)[ENTRY_STR]: _table_get_str,  \
        int(*)[ENTRY_PTR]: _table_get_void)((T), (K))

#define table_get(T, K, V)                                  \
    (^(table_t * _t, typeof(K) _k, typeof(V) _v) {          \
        uint64_t _out = 0;                                  \
        if (!_table_get(_t, _T_GET(_t, _k), &_out))         \
            return false;                                   \
        table_entry_t *_entry = (table_entry_t *)_out;      \
        if (!_entry)                                        \
            return false;                                   \
        if (_v)                                             \
            *_v = (typeof(*_v))_entry->value;               \
        return true;                                        \
    })((T), (K), (V))

#define table_has(T, K)                         \
    (^(table_t * _t, typeof(K) _k) {            \
        return _table_has(_t, _T_GET(_t, _k));  \
    })((T), (K))

#define table_del(T, K)                                             \
    (^(table_t * _t, typeof(K) _k) {                                \
        uint64_t _key = _T_GET(_t, _k);                             \
        return _table_has(_t, _key) ? _table_del(_t, _key) : false; \
    })((T), (K))

#define table_each(T, USERDATA, FN)                                                 \
    _Generic((FN),                                                                  \
        void(*)(table_t *, const char *, table_entry_t *, void *): _table_each_fn,  \
        void(^)(table_t *, const char *, table_entry_t *, void *): _table_each_block)((T), (FN), (USERDATA))

#define table_keys(T, USERDATA, FN)                                     \
    _Generic((FN),                                                      \
        void(*)(table_t *, const char *, void *): _table_keys_each_fn,  \
        void(^)(table_t *, const char *, void *): _table_keys_each_block)((T), (FN), (USERDATA))

// please ignore these, thank you x
uint64_t _table_murmur(const void *data, size_t len, uint32_t seed);
bool _table_set_int(table_t *table, uint64_t key, uint64_t value);
bool _table_set_str(table_t *table, const char *key, uint64_t value);
bool _table_set_void(table_t *table, void *key, uint64_t value);
uint64_t _table_int_to_int(table_t *_, uint64_t i);
uint64_t _table_str_to_int(table_t *table, const char *str);
uint64_t _table_void_to_int(table_t *_, void *ptr);
bool _table_set_int(table_t *table, uint64_t key, uint64_t value);
bool _table_set_str(table_t *table, const char *key, uint64_t value);
bool _table_set_void(table_t *table, void *key, uint64_t value);
int _table_get(table_t *table, uint64_t key, uint64_t *val);
uint64_t _table_get_int(table_t *table, uint64_t key);
uint64_t _table_get_str(table_t *table, const char *key);
uint64_t _table_get_void(table_t *table, void *key);
bool _table_has(table_t *table, uint64_t key);
bool _table_del(table_t *table, uint64_t key);
void _table_each_fn(table_t *table, void(*callback)(table_t*, const char*, table_entry_t*, void*), void *userdata);
void _table_each_block(table_t *table, void(^callback)(table_t*, const char*, table_entry_t*, void*), void *userdata);
imap_node_t* _imap_ensure(imap_node_t *tree, uint32_t n);

#ifdef __cplusplus
}
#endif
#endif // TABLE_HEADER

#ifdef TABLE_IMPLEMENTATION
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef __has_include
#define __has_include(x) 0
#endif
#if __has_include(<Block.h>)
#include <Block.h>
#endif

struct imap_node_t {
    union {
        uint32_t vec32[16];
        uint64_t vec64[8];
    };
};

#define imap__tree_root__           0
#define imap__tree_resv__           1
#define imap__tree_mark__           2
#define imap__tree_size__           3
#define imap__tree_nfre__           4
#define imap__tree_vfre__           5

#define imap__slot_pmask__          0x0000000f
#define imap__slot_node__           0x00000010
#define imap__slot_scalar__         0x00000020
#define imap__slot_value__          0xffffffe0
#define imap__slot_shift__          6
#define imap__slot_boxed__(sval)    (!((sval) & imap__slot_scalar__) && ((sval) >> imap__slot_shift__))

typedef struct {
    uint32_t stack[16];
    uint32_t stackp;
} imap_iter_t;

typedef struct {
    uint64_t x;
    uint32_t *slot;
} imap_pair_t;

#define imap__pair_zero__           ((imap_pair_t){0})
#define imap__pair__(x, slot)       ((imap_pair_t){(x), (slot)})
#define imap__node_zero__           ((imap_node_t){{{0}}})

#ifdef _MSC_VER
static inline uint32_t imap__bsr__(uint64_t x) {
    return _BitScanReverse64((unsigned long *)&x, x | 1), (unsigned long)x;
}
#else
static inline uint32_t imap__bsr__(uint64_t x) {
    return 63 - __builtin_clzll(x | 1);
}
#endif

static inline uint32_t imap__xpos__(uint64_t x) {
    return imap__bsr__(x) >> 2;
}

static inline uint64_t imap__ceilpow2__(uint64_t x) {
    return 1ull << (imap__bsr__(x - 1) + 1);
}

static inline void *imap__aligned_alloc__(uint64_t alignment, uint64_t size) {
    void *p = TABLE_MALLOC(size + sizeof(void *) + alignment - 1);
    if (!p)
        return p;
    void **ap = (void**)(((uint64_t)p + sizeof(void *) + alignment - 1) & ~(alignment - 1));
    ap[-1] = p;
    return ap;
}

static inline void imap__aligned_free__(void *p) {
    if (p)
        TABLE_FREE(((void**)p)[-1]);
}

#define IMAP_ALIGNED_ALLOC(a, s)    (imap__aligned_alloc__(a, s))
#define IMAP_ALIGNED_FREE(p)        (imap__aligned_free__(p))

static inline imap_node_t* imap__node__(imap_node_t *tree, uint32_t val) {
    return (imap_node_t*)((uint8_t*)tree + val);
}

static inline uint32_t imap__node_pos__(imap_node_t *node) {
    return node->vec32[0] & 0xf;
}

static inline uint64_t imap__extract_lo4_port__(uint32_t vec32[16]) {
    union {
        uint32_t *vec32;
        uint64_t *vec64;
    } u;
    u.vec32 = vec32;
    return
        ((u.vec64[0] & 0xf0000000full)) |
        ((u.vec64[1] & 0xf0000000full) << 4) |
        ((u.vec64[2] & 0xf0000000full) << 8) |
        ((u.vec64[3] & 0xf0000000full) << 12) |
        ((u.vec64[4] & 0xf0000000full) << 16) |
        ((u.vec64[5] & 0xf0000000full) << 20) |
        ((u.vec64[6] & 0xf0000000full) << 24) |
        ((u.vec64[7] & 0xf0000000full) << 28);
}

static inline void imap__deposit_lo4_port__(uint32_t vec32[16], uint64_t value) {
    union {
        uint32_t *vec32;
        uint64_t *vec64;
    } u;
    u.vec32 = vec32;
    u.vec64[0] = (u.vec64[0] & ~0xf0000000full) | ((value) & 0xf0000000full);
    u.vec64[1] = (u.vec64[1] & ~0xf0000000full) | ((value >> 4) & 0xf0000000full);
    u.vec64[2] = (u.vec64[2] & ~0xf0000000full) | ((value >> 8) & 0xf0000000full);
    u.vec64[3] = (u.vec64[3] & ~0xf0000000full) | ((value >> 12) & 0xf0000000full);
    u.vec64[4] = (u.vec64[4] & ~0xf0000000full) | ((value >> 16) & 0xf0000000full);
    u.vec64[5] = (u.vec64[5] & ~0xf0000000full) | ((value >> 20) & 0xf0000000full);
    u.vec64[6] = (u.vec64[6] & ~0xf0000000full) | ((value >> 24) & 0xf0000000full);
    u.vec64[7] = (u.vec64[7] & ~0xf0000000full) | ((value >> 28) & 0xf0000000full);
}

static inline void imap__node_setprefix__(imap_node_t *node, uint64_t prefix) {
    imap__deposit_lo4_port__(node->vec32, prefix);
}

static inline uint64_t imap__node_prefix__(imap_node_t *node) {
    return imap__extract_lo4_port__(node->vec32);
}

static inline uint32_t imap__xdir__(uint64_t x, uint32_t pos) {
    return (x >> (pos << 2)) & 0xf;
}

static inline uint32_t imap__popcnt_hi28_port__(uint32_t vec32[16], uint32_t *p) {
    uint32_t pcnt = 0, sval, dirn;
    *p = 0;
    for (dirn = 0; 16 > dirn; dirn++)
    {
        sval = vec32[dirn];
        if (sval & ~0xf)
        {
            *p = sval;
            pcnt++;
        }
    }
    return pcnt;
}

static inline uint32_t imap__node_popcnt__(imap_node_t *node, uint32_t *p) {
    return imap__popcnt_hi28_port__(node->vec32, p);
}

static inline uint32_t imap__alloc_node__(imap_node_t *tree) {
    uint32_t mark = tree->vec32[imap__tree_nfre__];
    if (mark)
        tree->vec32[imap__tree_nfre__] = *(uint32_t*)((uint8_t *)tree + mark);
    else {
        mark = tree->vec32[imap__tree_mark__];
        assert(mark + sizeof(imap_node_t) <= tree->vec32[imap__tree_size__]);
        tree->vec32[imap__tree_mark__] = mark + sizeof(imap_node_t);
    }
    return mark;
}

static inline void imap__free_node__(imap_node_t *tree, uint32_t mark) {
    *(uint32_t *)((uint8_t *)tree + mark) = tree->vec32[imap__tree_nfre__];
    tree->vec32[imap__tree_nfre__] = mark;
}

static inline uint64_t imap__xpfx__(uint64_t x, uint32_t pos) {
    return x & (~0xfull << (pos << 2));
}

imap_node_t* _imap_ensure(imap_node_t *tree, uint32_t n) {
    imap_node_t *newtree;
    uint32_t hasnfre, hasvfre, newmark, oldsize, newsize;
    uint64_t newsize64;
    if (0 == n)
        return tree;
    if (0 == tree)
    {
        hasnfre = 0;
        hasvfre = 1;
        newmark = sizeof(imap_node_t);
        oldsize = 0;
    }
    else
    {
        hasnfre = !!tree->vec32[imap__tree_nfre__];
        hasvfre = !!tree->vec32[imap__tree_vfre__];
        newmark = tree->vec32[imap__tree_mark__];
        oldsize = tree->vec32[imap__tree_size__];
    }
    newmark += (n * 2 - hasnfre) * sizeof(imap_node_t) + (n - hasvfre) * sizeof(uint64_t);
    if (newmark <= oldsize)
        return tree;
    newsize64 = imap__ceilpow2__(newmark);
    if (0x20000000 < newsize64)
        return 0;
    newsize = (uint32_t)newsize64;
    newtree = (imap_node_t *)IMAP_ALIGNED_ALLOC(sizeof(imap_node_t), newsize);
    if (!newtree)
        return newtree;
    if (!tree) {
        newtree->vec32[imap__tree_root__] = 0;
        newtree->vec32[imap__tree_resv__] = 0;
        newtree->vec32[imap__tree_mark__] = sizeof(imap_node_t);
        newtree->vec32[imap__tree_size__] = newsize;
        newtree->vec32[imap__tree_nfre__] = 0;
        newtree->vec32[imap__tree_vfre__] = 3 << imap__slot_shift__;
        newtree->vec64[3] = 4 << imap__slot_shift__;
        newtree->vec64[4] = 5 << imap__slot_shift__;
        newtree->vec64[5] = 6 << imap__slot_shift__;
        newtree->vec64[6] = 7 << imap__slot_shift__;
        newtree->vec64[7] = 0;
    } else {
        memcpy(newtree, tree, tree->vec32[imap__tree_mark__]);
        IMAP_ALIGNED_FREE(tree);
        newtree->vec32[imap__tree_size__] = newsize;
    }
    return newtree;
}

static uint32_t *imap_lookup(imap_node_t *tree, uint64_t x) {
    imap_node_t *node = tree;
    uint32_t *slot;
    uint32_t sval, posn = 16, dirn = 0;
    for (;;) {
        slot = &node->vec32[dirn];
        sval = *slot;
        if (!(sval & imap__slot_node__)) {
            if ((sval & imap__slot_value__) && imap__node_prefix__(node) == (x & ~0xfull)) {
                assert(0 == posn);
                return slot;
            }
            return 0;
        }
        node = imap__node__(tree, sval & imap__slot_value__);
        posn = imap__node_pos__(node);
        dirn = imap__xdir__(x, posn);
    }
}

static uint32_t *imap_assign(imap_node_t *tree, uint64_t x) {
    uint32_t *slotstack[16 + 1];
    uint32_t posnstack[16 + 1];
    uint32_t stackp, stacki;
    imap_node_t *newnode, *node = tree;
    uint32_t *slot;
    uint32_t newmark, sval, diff, posn = 16, dirn = 0;
    uint64_t prfx;
    stackp = 0;
    for (;;) {
        slot = &node->vec32[dirn];
        sval = *slot;
        slotstack[stackp] = slot, posnstack[stackp++] = posn;
        if (!(sval & imap__slot_node__)) {
            prfx = imap__node_prefix__(node);
            if (0 == posn && prfx == (x & ~0xfull))
                return slot;
            diff = imap__xpos__(prfx ^ x);
            assert(diff < 16);
            for (stacki = stackp; diff > posn;)
                posn = posnstack[--stacki];
            if (stacki != stackp) {
                slot = slotstack[stacki];
                sval = *slot;
                assert(sval & imap__slot_node__);
                newmark = imap__alloc_node__(tree);
                *slot = (*slot & imap__slot_pmask__) | imap__slot_node__ | newmark;
                newnode = imap__node__(tree, newmark);
                *newnode = imap__node_zero__;
                newmark = imap__alloc_node__(tree);
                newnode->vec32[imap__xdir__(prfx, diff)] = sval;
                newnode->vec32[imap__xdir__(x, diff)] = imap__slot_node__ | newmark;
                imap__node_setprefix__(newnode, imap__xpfx__(prfx, diff) | diff);
            } else {
                newmark = imap__alloc_node__(tree);
                *slot = (*slot & imap__slot_pmask__) | imap__slot_node__ | newmark;
            }
            newnode = imap__node__(tree, newmark);
            *newnode = imap__node_zero__;
            imap__node_setprefix__(newnode, x & ~0xfull);
            return &newnode->vec32[x & 0xfull];
        }
        node = imap__node__(tree, sval & imap__slot_value__);
        posn = imap__node_pos__(node);
        dirn = imap__xdir__(x, posn);
    }
}

static inline uint32_t imap__alloc_val__(imap_node_t *tree) {
    uint32_t mark = imap__alloc_node__(tree);
    imap_node_t *node = imap__node__(tree, mark);
    mark <<= 3;
    tree->vec32[imap__tree_vfre__] = mark;
    node->vec64[0] = mark + (1 << imap__slot_shift__);
    node->vec64[1] = mark + (2 << imap__slot_shift__);
    node->vec64[2] = mark + (3 << imap__slot_shift__);
    node->vec64[3] = mark + (4 << imap__slot_shift__);
    node->vec64[4] = mark + (5 << imap__slot_shift__);
    node->vec64[5] = mark + (6 << imap__slot_shift__);
    node->vec64[6] = mark + (7 << imap__slot_shift__);
    node->vec64[7] = 0;
    return mark;
}

static uint64_t imap_getval64(imap_node_t *tree, uint32_t *slot) {
    assert(!(*slot & imap__slot_node__));
    uint32_t sval = *slot;
    return tree->vec64[sval >> imap__slot_shift__];
}

static void imap_setval64(imap_node_t *tree, uint32_t *slot, uint64_t y) {
    assert(!(*slot & imap__slot_node__));
    uint32_t sval = *slot;
    if (!(sval >> imap__slot_shift__))
    {
        sval = tree->vec32[imap__tree_vfre__];
        if (!sval)
            sval = imap__alloc_val__(tree);
        assert(sval >> imap__slot_shift__);
        tree->vec32[imap__tree_vfre__] = (uint32_t)tree->vec64[sval >> imap__slot_shift__];
    }
    assert(!(sval & imap__slot_node__));
    assert(imap__slot_boxed__(sval));
    *slot = (*slot & imap__slot_pmask__) | sval;
    tree->vec64[sval >> imap__slot_shift__] = y;
}

static void imap_delval(imap_node_t *tree, uint32_t *slot) {
    assert(!(*slot & imap__slot_node__));
    uint32_t sval = *slot;
    if (imap__slot_boxed__(sval)) {
        tree->vec64[sval >> imap__slot_shift__] = tree->vec32[imap__tree_vfre__];
        tree->vec32[imap__tree_vfre__] = sval & imap__slot_value__;
    }
    *slot &= imap__slot_pmask__;
}

static void imap_remove(imap_node_t *tree, uint64_t x) {
    uint32_t *slotstack[16 + 1];
    uint32_t stackp;
    imap_node_t *node = tree;
    uint32_t *slot;
    uint32_t sval, pval, posn = 16, dirn = 0;
    stackp = 0;
    for (;;) {
        slot = &node->vec32[dirn];
        sval = *slot;
        if (!(sval & imap__slot_node__)) {
            if ((sval & imap__slot_value__) && imap__node_prefix__(node) == (x & ~0xfull)) {
                assert(0 == posn);
                imap_delval(tree, slot);
            }
            while (stackp) {
                slot = slotstack[--stackp];
                sval = *slot;
                node = imap__node__(tree, sval & imap__slot_value__);
                posn = imap__node_pos__(node);
                if (!!posn != imap__node_popcnt__(node, &pval))
                    break;
                imap__free_node__(tree, sval & imap__slot_value__);
                *slot = (sval & imap__slot_pmask__) | (pval & ~imap__slot_pmask__);
            }
            return;
        }
        node = imap__node__(tree, sval & imap__slot_value__);
        posn = imap__node_pos__(node);
        dirn = imap__xdir__(x, posn);
        slotstack[stackp++] = slot;
    }
}

static imap_pair_t imap_iterate(imap_node_t *tree, imap_iter_t *iter, int restart) {
    imap_node_t *node;
    uint32_t *slot;
    uint32_t sval, dirn;
    if (restart) {
        iter->stackp = 0;
        sval = dirn = 0;
        goto enter;
    }
    // loop while stack is not empty
    while (iter->stackp) {
        // get slot value and increment direction
        sval = iter->stack[iter->stackp - 1]++;
        dirn = sval & 31;
        if (15 < dirn) {
            // if directions 0-15 have been examined, pop node from stack
            iter->stackp--;
            continue;
        }
    enter:
        node = imap__node__(tree, sval & imap__slot_value__);
        slot = &node->vec32[dirn];
        sval = *slot;
        if (sval & imap__slot_node__)
            // push node into stack
            iter->stack[iter->stackp++] = sval & imap__slot_value__;
        else if (sval & imap__slot_value__)
            return imap__pair__(imap__node_prefix__(node) | dirn, slot);
    }
    return imap__pair_zero__;
}

#define ROTL32(x, r) ((x << r) | (x >> (32 - r)))
#define FMIX32(h) h^=h>>16; h*=0x85ebca6b; h^=h>>13; h*=0xc2b2ae35; h^=h>>16;

static void MM86128(const void *key, const int len, uint32_t seed, void *out) {
    const uint8_t * data = (const uint8_t*)key;
    const int nblocks = len / 16;
    uint32_t h1 = seed;
    uint32_t h2 = seed;
    uint32_t h3 = seed;
    uint32_t h4 = seed;
    uint32_t c1 = 0x239b961b;
    uint32_t c2 = 0xab0e9789;
    uint32_t c3 = 0x38b34ae5;
    uint32_t c4 = 0xa1e38b93;
    const uint32_t * blocks = (const uint32_t *)(data + nblocks*16);
    for (int i = -nblocks; i; i++) {
        uint32_t k1 = blocks[i*4+0];
        uint32_t k2 = blocks[i*4+1];
        uint32_t k3 = blocks[i*4+2];
        uint32_t k4 = blocks[i*4+3];
        k1 *= c1; k1  = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
        h1 = ROTL32(h1,19); h1 += h2; h1 = h1*5+0x561ccd1b;
        k2 *= c2; k2  = ROTL32(k2,16); k2 *= c3; h2 ^= k2;
        h2 = ROTL32(h2,17); h2 += h3; h2 = h2*5+0x0bcaa747;
        k3 *= c3; k3  = ROTL32(k3,17); k3 *= c4; h3 ^= k3;
        h3 = ROTL32(h3,15); h3 += h4; h3 = h3*5+0x96cd1c35;
        k4 *= c4; k4  = ROTL32(k4,18); k4 *= c1; h4 ^= k4;
        h4 = ROTL32(h4,13); h4 += h1; h4 = h4*5+0x32ac3b17;
    }
    const uint8_t * tail = (const uint8_t*)(data + nblocks*16);
    uint32_t k1 = 0;
    uint32_t k2 = 0;
    uint32_t k3 = 0;
    uint32_t k4 = 0;
    switch(len & 15) {
        case 15:
            k4 ^= tail[14] << 16;
        case 14:
            k4 ^= tail[13] << 8;
        case 13:
            k4 ^= tail[12] << 0;
            k4 *= c4; k4  = ROTL32(k4,18); k4 *= c1; h4 ^= k4;
        case 12:
            k3 ^= tail[11] << 24;
        case 11:
            k3 ^= tail[10] << 16;
        case 10:
            k3 ^= tail[ 9] << 8;
        case 9:
            k3 ^= tail[ 8] << 0;
            k3 *= c3; k3  = ROTL32(k3,17); k3 *= c4; h3 ^= k3;
        case 8:
            k2 ^= tail[ 7] << 24;
        case 7:
            k2 ^= tail[ 6] << 16;
        case 6:
            k2 ^= tail[ 5] << 8;
        case 5:
            k2 ^= tail[ 4] << 0;
            k2 *= c2; k2  = ROTL32(k2,16); k2 *= c3; h2 ^= k2;
        case 4:
            k1 ^= tail[ 3] << 24;
        case 3:
            k1 ^= tail[ 2] << 16;
        case 2:
            k1 ^= tail[ 1] << 8;
        case 1:
            k1 ^= tail[ 0] << 0;
            k1 *= c1; k1  = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
    };
    h1 ^= len; h2 ^= len; h3 ^= len; h4 ^= len;
    h1 += h2; h1 += h3; h1 += h4;
    h2 += h1; h3 += h1; h4 += h1;
    FMIX32(h1); FMIX32(h2); FMIX32(h3); FMIX32(h4);
    h1 += h2; h1 += h3; h1 += h4;
    h2 += h1; h3 += h1; h4 += h1;
    ((uint32_t*)out)[0] = h1;
    ((uint32_t*)out)[1] = h2;
    ((uint32_t*)out)[2] = h3;
    ((uint32_t*)out)[3] = h4;
}

uint64_t _table_murmur(const void *data, size_t len, uint32_t seed) {
    char out[16];
    MM86128(data, (int)len, (uint32_t)seed, &out);
    return *(uint64_t*)out;
}

#define _ENTRY(T, V)                                                \
    do                                                              \
    {                                                               \
        table_entry_t *entry = TABLE_MALLOC(sizeof(table_entry_t)); \
        entry->type = (T);                                          \
        entry->value = (V);                                         \
        return (uintptr_t)entry;                                    \
    } while (0)

uint64_t _table_int_to_int(table_t *table, uint64_t i) {
    _ENTRY(ENTRY_INT, i);
}

uint64_t _table_str_to_int(table_t *table, const char *str) {
    _ENTRY(ENTRY_STR, (uintptr_t)strdup(str));
}

uint64_t _table_void_to_int(table_t *table, void *ptr) {
    _ENTRY(ENTRY_PTR, (uintptr_t)ptr);
}

static inline bool imap_set(imap_t *map, uint64_t key, uint64_t value) {
    uint32_t *slot = imap_lookup(map->tree, key);
    if (slot) {
        imap_setval64(map->tree, slot, value);
        return true;
    }
    if (map->count + 1 >= map->capacity) {
        map->capacity *= 2;
        map->tree = _imap_ensure(map->tree, map->capacity);
    }
    if (!(slot = imap_assign(map->tree, key)))
        return false;
    imap_setval64(map->tree, slot, value);
    map->count++;
    return true;
}

bool _table_set_int(table_t *table, uint64_t key, uint64_t value) {
    return imap_set(&table->map, key, value);
}

#define _HASH(T, STR) (!(T)->hashfn ? -1LL : (T)->hashfn((void*)(STR), strlen((STR)), (T)->seed))

bool _table_set_str(table_t *table, const char *key, uint64_t value) {
    uint64_t key_int = _HASH(table, key);
    imap_set(&table->keys, key_int, (uintptr_t)strdup(key));
    return key_int == (uintptr_t)NULL ? false : _table_set_int(table, key_int, value);
}

bool _table_set_void(table_t *table, void *key, uint64_t value) {
    return _table_set_int(table, (uintptr_t)key, value);
}

int _table_get(table_t *table, uint64_t key, uint64_t *val) {
    uint32_t *slot = imap_lookup(table->map.tree, key);
    if (!slot)
        return 0;
    if (val)
        *val = imap_getval64(table->map.tree, slot);
    return 1;
}

uint64_t _table_get_int(table_t *table, uint64_t key) {
    return key;
}

uint64_t _table_get_str(table_t *table, const char *key) {
    return _HASH(table, key);
}

uint64_t _table_get_void(table_t *table, void *key) {
    return (uintptr_t)key;
}

bool _table_has(table_t *table, uint64_t key) {
    return imap_lookup(table->map.tree, key) != NULL;
}

bool _table_del(table_t *table, uint64_t key) {
    imap_remove(table->map.tree, key);
    table->map.count--;
    return true;
}

void table_free(table_t *table) {
    if (!table)
        return;
    if (table->map.tree) {
        table_each(table, NULL, ^(table_t *table, const char *key, table_entry_t *entry, void *userdata) {
            if (entry->type == ENTRY_STR)
                free((void*)entry->value);
        });
        IMAP_ALIGNED_FREE(table->map.tree);
        imap_iter_t iter;
        imap_pair_t pair = imap_iterate(table->keys.tree, &iter, 1);
        while (pair.slot) {
            uint64_t value = 0;
            _table_get(table, pair.x, &value);
            free((void*)value);
            pair = imap_iterate(table->keys.tree, &iter, 0);
        }
        IMAP_ALIGNED_FREE(table->keys.tree);
    }
    memset(table, 0, sizeof(table_t));
}

#define _T_ITER(T, CB, UD)                                                 \
    do                                                                     \
    {                                                                      \
        imap_iter_t iter;                                                  \
        imap_pair_t pair = imap_iterate((T)->map.tree, &iter, 1);          \
        while (pair.slot)                                                  \
        {                                                                  \
            uint64_t value = 0;                                            \
            _table_get((T), pair.x, &value);                               \
            const char *key = NULL;                                        \
            uint32_t *slot = imap_lookup(table->keys.tree, pair.x);        \
            if (slot)                                                      \
                key = (const char *)imap_getval64(table->keys.tree, slot); \
            CB(table, key, (table_entry_t *)value, (UD));                  \
            pair = imap_iterate((T)->map.tree, &iter, 0);                  \
        }                                                                  \
    } while (0)

void _table_each_fn(table_t *table, void(*callback)(table_t*, const char*, table_entry_t*, void*), void *userdata) {
    _T_ITER(table, callback, userdata);  
}

void _table_each_block(table_t *table, void(^callback)(table_t*, const char*, table_entry_t*, void*), void *userdata) {
    _T_ITER(table, callback, userdata);  
}

#define _T_KEYS_ITER(T, CB, UD)                                                    \
    do                                                                             \
    {                                                                              \
        imap_iter_t iter;                                                          \
        imap_pair_t pair = imap_iterate((T)->keys.tree, &iter, 1);                 \
        while (pair.slot)                                                          \
        {                                                                          \
            uint32_t *slot = imap_lookup(table->keys.tree, pair.x);                \
            const char *key = (const char *)imap_getval64(table->keys.tree, slot); \
            CB((T), key, (UD));                                                    \
            pair = imap_iterate((T)->map.tree, &iter, 0);                          \
        }                                                                          \
    } while (0)

void _table_keys_each_fn(table_t *table, void(*callback)(table_t*, const char*, void*), void *userdata) {
    _T_KEYS_ITER(table, callback, userdata);  
}

void _table_keys_each_block(table_t *table, void(^callback)(table_t*, const char*, void*), void *userdata) {
    _T_KEYS_ITER(table, callback, userdata);  
}
#endif
