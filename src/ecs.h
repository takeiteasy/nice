//
//  ecs.h
//  rpg
//
//  Created by George Watson on 22/07/2025.
//

#pragma once

#include <stdint.h>
#include <stddef.h>

typedef union entity {
    struct {
        uint32_t id;
        uint16_t version;
        uint8_t alive;
        uint8_t type;
    };
    uint64_t value;
} entity;

struct sparse {
    entity *sparse;
    entity *dense;
    size_t sizeOfSparse;
    size_t sizeOfDense;
};

struct storage {
    entity componentId;
    void *data;
    size_t sizeOfData;
    size_t sizeOfComponent;
    struct sparse *sparse;
};

struct world {
    struct storage **storages;
    struct storage *systems;
    size_t sizeOfStorages;
    entity *entities;
    size_t sizeOfEntities;
    uint32_t *recyclable;
    size_t sizeOfRecyclable;
};

extern const uint64_t ecs_nil;
extern const entity ecs_nil_entity;

typedef struct world world_t;
typedef void(^system_t)(entity);
typedef int(^filter_system_t)(entity);

enum {
    ECS_ENTITY,
    ECS_COMPONENT,
    ECS_SYSTEM
};

world_t* ecs_world(void);
void ecs_world_destroy(world_t **world);

entity ecs_spawn(world_t *world);
entity ecs_component(world_t *world, size_t size_of_component);
entity ecs_system(world_t *world, system_t fn, int component_count, ...);
void ecs_delete(world_t *world, entity e);

int entity_isvalid(world_t *world, entity e);
int entity_isa(world_t *world, entity e, int type);
int entity_cmp(entity a, entity b);
int entity_isnil(entity e);
void* entity_give(world_t *world, entity e, entity c);
void entity_remove(world_t *world, entity e, entity c);
int entity_has(world_t *world, entity e, entity c);
void* entity_get(world_t *world, entity e, entity c);
void entity_set(world_t *world, entity e, entity c, void *data);

void ecs_step(world_t *world);
entity* ecs_find(world_t *world, filter_system_t filter, int *result_count, int component_count, ...);
void ecs_query(world_t *world, system_t fn, filter_system_t filter, int component_count, ...);
