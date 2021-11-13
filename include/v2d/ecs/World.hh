#pragma once

#include <v2d/ecs/Entity.hh>
#include <v2d/ecs/System.hh>

namespace v2d {

struct World : public EntityManager, public SystemManager {
    void update(float dt);
};

} // namespace v2d
