//
//  ecs.c
//  rpg
//
//  Created by George Watson on 22/07/2025.
//

#include "ecs.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

const uint64_t ecs_nil = 0xFFFFFFFFull;
const union entity ecs_nil_entity = {.id=ecs_nil};

static struct sparse* sparse(void) {
    struct sparse *result = malloc(sizeof(struct sparse));
    memset(result, 0, sizeof(struct sparse));
    return result;
}

static void sparse_destroy(struct sparse **sparse) {
    struct sparse *s = *sparse;
    if (s->sparse)
        free(s->sparse);
    if (s->dense)
        free(s->dense);
    free(s);
    *sparse = NULL;
}

static int sparse_has(struct sparse *sparse, union entity e) {
    return e.id < sparse->sizeOfSparse && !entity_isnil(sparse->sparse[e.id]);
}

static void sparse_emplace(struct sparse *sparse, union entity e) {
    if (e.id > sparse->sizeOfSparse) {
        size_t size = e.id + 1;
        sparse->sparse = realloc(sparse->sparse, size * sizeof * sparse->sparse);
        for (size_t i = sparse->sizeOfSparse; i < size; i++)
            sparse->sparse[i] = ecs_nil_entity;
        sparse->sizeOfSparse = size;
    }
    sparse->sparse[e.id] = (union entity){.id=(uint32_t)sparse->sizeOfDense};
    sparse->dense = realloc(sparse->dense, (sparse->sizeOfDense + 1) * sizeof * sparse->dense);
    sparse->dense[sparse->sizeOfDense++] = e;
}

static size_t sparse_at(struct sparse *sparse, union entity e) {
    return sparse->sparse[e.id].id;
}

static size_t sparse_remove(struct sparse *sparse, union entity e) {
    assert(sparse_has(sparse, e));
    uint32_t pos = sparse_at(sparse, e);
    union entity other = sparse->dense[sparse->sizeOfDense-1];
    sparse->sparse[other.id] = (union entity){.id=pos};
    sparse->dense[pos] = other;
    sparse->sparse[e.id] = ecs_nil_entity;
    sparse->dense = realloc(sparse->dense, --sparse->sizeOfDense * sizeof * sparse->dense);
    return pos;
}

static struct storage* storage(union entity e, size_t sz) {
    struct storage *storage = malloc(sizeof(struct storage));
    *storage = (struct storage) {
        .componentId = e,
        .sizeOfComponent = sz,
        .sizeOfData = 0,
        .data = NULL,
        .sparse = sparse()
    };
    return storage;
}

static void storage_destroy(struct storage **strg) {
    struct storage* s = *strg;
    if (s->sparse)
        sparse_destroy(&s->sparse);
    if (s->data)
        free(s->data);
    free(s);
    *strg = NULL;
}

static int storage_has(struct storage *strg, union entity e) {
    return sparse_has(strg->sparse, e);
}

static void* storage_emplace(struct storage *strg, union entity e) {
    strg->data = realloc(strg->data, ++strg->sizeOfData * sizeof(char) * strg->sizeOfComponent);
    void *result = &((char*)strg->data)[(strg->sizeOfData - 1) * sizeof(char) * strg->sizeOfComponent];
    sparse_emplace(strg->sparse, e);
    return result;
}

static void storage_remove(struct storage *strg, union entity e) {
    size_t pos = sparse_remove(strg->sparse, e);
    memmove(&((char*)strg->data)[pos * sizeof(char) * strg->sizeOfComponent],
            &((char*)strg->data)[(strg->sizeOfData - 1) * sizeof(char) * strg->sizeOfComponent],
            strg->sizeOfComponent);
    strg->data = realloc(strg->data, --strg->sizeOfData * sizeof(char) * strg->sizeOfComponent);
}

static void* storage_get(struct storage *strg, union entity e) {
    uint32_t pos = sparse_at(strg->sparse, e);
    return &((char*)strg->data)[pos * sizeof(char) * strg->sizeOfComponent];
}

struct system {
    uint32_t id;
    uint32_t component_count;
    union entity *components;
    system_t callback;
};

static union entity make_entity(struct world *world, uint8_t type) {
    if (world->sizeOfRecyclable) {
        uint32_t id = world->recyclable[world->sizeOfRecyclable-1];
        union entity old = world->entities[id];
        union entity new = (union entity) {
            .id = old.id,
            .version = old.version,
            .alive = 1,
            .type = type
        };
        world->entities[id] = new;
        world->recyclable = realloc(world->recyclable, --world->sizeOfRecyclable * sizeof(uint32_t));
        return new;
    } else {
        world->entities = realloc(world->entities, ++world->sizeOfEntities * sizeof(union entity));
        union entity e = (union entity) {
            .id = (uint32_t)world->sizeOfEntities - 1,
            .version = 0,
            .alive = 1,
            .type = type
        };
        world->entities[e.id] = e;
        return e;
    }
}

struct world* ecs_world_create(void) {
    struct world *result = malloc(sizeof(struct world));
    memset(result, 0, sizeof(struct world));
    union entity e = make_entity(result, ECS_COMPONENT); // doesn't matter will always be first entity
    result->systems = storage(e, sizeof(struct system));
    return result;
}

void ecs_world_destroy(struct world **_world) {
    struct world *world = *_world;
    if (world->storages) {
        for (int i = 0; i < world->sizeOfStorages; i++)
            storage_destroy(&world->storages[i]);
        free(world->storages);
    }
    if (world->entities)
        free(world->entities);
    if (world->recyclable)
        free(world->recyclable);
    if (world->systems)
        storage_destroy(&world->systems);
    free(world);
    *_world = NULL;
}

static struct storage* find_storage(struct world *world, union entity e) {
    for (int i = 0; i < world->sizeOfStorages; i++)
        if (entity_cmp(e, world->storages[i]->componentId))
            return world->storages[i];
    return NULL;
}

union entity ecs_spawn(struct world *world) {
    return make_entity(world, ECS_ENTITY);
}

union entity ecs_component(struct world *world, size_t sizeOfComponent) {
    union entity e = make_entity(world, ECS_COMPONENT);
    struct storage *strg = find_storage(world, e);
    if (strg)
        return e;
    strg = storage(e, sizeOfComponent);
    world->storages = realloc(world->storages, (world->sizeOfStorages + 1) * sizeof * world->storages);
    world->storages[world->sizeOfStorages++] = strg;
    return e;
}

static union entity* vargs_components(struct world *world, int n, va_list args) {
    union entity *result = malloc(n * sizeof(union entity));
    for (int i = 0; i < n; i++) {
        union entity component = va_arg(args, union entity);
        if (!entity_isa(world, component, ECS_COMPONENT)) {
            free(result);
            result = NULL;
            goto BAIL;
        }
        result[i] = component;
    }
BAIL:
    va_end(args);
    return result;
}

union entity ecs_system(struct world *world, system_t system, int n, ...) {
    union entity e = make_entity(world, ECS_SYSTEM);
    va_list args;
    va_start(args, n);
    struct system *system_data = malloc(sizeof(struct system));
    system_data->id = e.id;
    system_data->component_count = n;
    system_data->components = vargs_components(world, n, args);
    system_data->callback = system;
    void *ptr = storage_emplace(world->systems, e);
    memcpy(ptr, system_data, sizeof(struct system));
    return e;
}

void ecs_delete(struct world *world, union entity e) {
    switch (e.type) {
        case ECS_ENTITY:
            for (size_t i = world->sizeOfStorages; i; --i)
                if (world->storages[i - 1] && sparse_has(world->storages[i - 1]->sparse, e))
                    storage_remove(world->storages[i - 1], e);
            world->entities[e.id] = (union entity) {
                .id = e.id,
                .version = e.version + 1,
                .alive = 0,
                .type = 255
            };
            world->recyclable = realloc(world->recyclable, ++world->sizeOfRecyclable * sizeof(uint32_t));
            world->recyclable[world->sizeOfRecyclable-1] = e.id;
            break;
        case ECS_COMPONENT:
            // TODO: !
            break;
        case ECS_SYSTEM:
            // TODO: !
            break;
    }
}

int entity_isvalid(struct world *world, union entity e) {
    return world->sizeOfEntities > e.id && entity_cmp(world->entities[e.id], e);
}

int entity_isa(struct world *world, union entity e, int type) {
    return entity_isvalid(world, e) && e.type == type;
}

int entity_cmp(union entity a, union entity b) {
    return a.value == b.value;
}

int entity_isnil(union entity e) {
    return e.id == ecs_nil;
}

static struct storage* find_entity_storage(struct world *world, union entity e, union entity c) {
    assert(entity_isa(world, e, ECS_ENTITY));
    assert(entity_isa(world, c, ECS_COMPONENT));
    struct storage *strg = find_storage(world, c);
    assert(strg);
    return strg;
}

void* entity_give(struct world *world, union entity e, union entity c) {
    return storage_emplace(find_entity_storage(world, e, c), e);
}

void entity_remove(struct world *world, union entity e, union entity c) {
    struct storage *strg = find_entity_storage(world, e, c);
    assert(storage_has(strg, e));
    storage_remove(strg, e);
}

void* entity_get(struct world *world, union entity e, union entity c) {
    struct storage *strg = find_entity_storage(world, e, c);
    return storage_has(strg, e) ? storage_get(strg, e) : NULL;
}

void entity_set(struct world *world, union entity e, union entity c, void *data) {
    struct storage *strg = find_entity_storage(world, e, c);
    memcpy(storage_has(strg, e) ? storage_get(strg, e) : storage_emplace(strg, e),
           data,
           strg->sizeOfComponent);
}

int entity_has(struct world *world, union entity e, union entity c) {
    struct storage *strg = find_storage(world, c);
    if (!strg)
        return 0;
    return storage_has(strg, e);
}

union entity* ecs_find(struct world *world, filter_system_t filter, int *result_count, int n, ...) {
    va_list args;
    va_start(args, n);
    union entity *components = vargs_components(world, n, args);
    union entity *result = NULL;
    int count = 0;
    for (int i = 0; i < world->sizeOfEntities; i++) {
        int match = 1;
        for (int j = 0; j < n; j++) {
            struct storage *strg = find_storage(world, components[j]);
            assert(strg);
            if (!storage_has(strg, world->entities[i])) {
                match = 0;
                break;
            }
        }
        if (match && !(filter && !filter(world->entities[i]))) {
            result = realloc(result, count + 1 * sizeof(union entity));
            result[count++] = world->entities[i];
        }
    }
    if (result_count)
        *result_count = count;
    return result;
}

void ecs_query(struct world *world, system_t fn, filter_system_t filter, int n, ...) {
    va_list args;
    va_start(args, n);
    union entity *components = vargs_components(world, n, args);

    for (int i = 0; i < world->sizeOfEntities; i++) {
        int match = 1;
        for (int j = 0; j < n; j++) {
            struct storage *strg = find_storage(world, components[j]);
            assert(strg);
            if (!storage_has(strg, world->entities[i])) {
                match = 0;
                break;
            }
        }
        if (match && !(filter && !filter(world->entities[i])))
            fn(world->entities[i]);
    }
}

void ecs_step(struct world *world) {
    for (int i = 0; i < world->systems->sparse->sizeOfDense; i++) {
        struct system *system_data = storage_get(world->systems, world->systems->sparse->dense[i]);
        union entity system_entity = world->entities[system_data->id];
        assert(entity_isa(world, system_entity, ECS_SYSTEM));
        if (!system_entity.alive)
            continue;

        for (int j = 0; j < world->sizeOfEntities; j++) {
            int match = 1;
            for (int k = 0; k < system_data->component_count; k++) {
                struct storage *strg = find_storage(world, system_data->components[k]);
                assert(strg);
                if (!storage_has(strg, world->entities[j])) {
                    match = 0;
                    break;
                }
            }
            if (match)
                system_data->callback(world->entities[j]);
        }
    }
}
