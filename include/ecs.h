//
//  ecs.h
//  rpg
//
//  Created by George Watson on 22/07/2025.
//

#pragma once

#include <stdint.h>
#include <stddef.h>

union entity {
    struct {
        uint32_t id;
        uint16_t version;
        uint8_t alive;
        uint8_t type;
    };
    uint64_t value;
};

struct sparse {
    union entity *sparse;
    union entity *dense;
    size_t sizeOfSparse;
    size_t sizeOfDense;
};

struct storage {
    union entity componentId;
    void *data;
    size_t sizeOfData;
    size_t sizeOfComponent;
    struct sparse *sparse;
};

struct world {
    struct storage **storages;
    struct storage *systems;
    size_t sizeOfStorages;
    union entity *entities;
    size_t sizeOfEntities;
    uint32_t *recyclable;
    size_t sizeOfRecyclable;
};

extern const uint64_t ecs_nil;
extern const union entity ecs_nil_entity;

typedef void(^system_t)(union entity);
typedef int(^filter_system_t)(union entity);

enum {
    ECS_ENTITY,
    ECS_COMPONENT,
    ECS_SYSTEM
};

struct world* ecs_world_create(void);
void ecs_world_destroy(struct world **world);

union entity ecs_spawn(struct world *world);
union entity ecs_component(struct world *world, size_t size_of_component);
union entity ecs_system(struct world *world, system_t fn, int component_count, ...);
void ecs_delete(struct world *world, union entity e);

int entity_isvalid(struct world *world, union entity e);
int entity_isa(struct world *world, union entity e, int type);
int entity_cmp(union entity a, union entity b);
int entity_isnil(union entity e);
void* entity_give(struct world *world, union entity e, union entity c);
void entity_remove(struct world *world, union entity e, union entity c);
int entity_has(struct world *world, union entity e, union entity c);
void* entity_get(struct world *world, union entity e, union entity c);
void entity_set(struct world *world, union entity e, union entity c, void *data);

void ecs_step(struct world *world);
union entity* ecs_find(struct world *world, filter_system_t filter, int *result_count, int component_count, ...);
void ecs_query(struct world *world, system_t fn, filter_system_t filter, int component_count, ...);
