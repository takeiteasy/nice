//
//  renderable.cpp
//  ice
//
//  Created by George Watson on 28/08/2025.
//

#include "world.hpp"

ECS_STRUCT(Renderable, {
    bool dummy;
});

#define $Renderables Renderables::instance()

class Renderables: public Global<Renderables> {
};

void RenderableImport(ecs_world_t *w) {
    ECS_MODULE(w, Renderable);
    ECS_IMPORT(w, FlecsMeta);
    ecs_entity_t scope = ecs_set_scope(w, 0);

    ECS_META_COMPONENT(w, Renderable);
    Renderable comp_value = { true };
    ecs_set_ptr(w, 0, Renderable, &comp_value);

    ecs_set_scope(w, scope);
}
