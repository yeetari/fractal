#pragma once

#include <v2d/ecs/System.hh>

namespace v2d {

class PhysicsSystem final : public System {
public:
    void update(World *world, float dt) override;
};

} // namespace v2d
