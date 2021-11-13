#pragma once

#include <v2d/ecs/System.hh>
#include <v2d/maths/Vec.hh>

namespace v2d {

struct ObjectData {
    Vec2f position;
    Vec2f scale;
    Vec2f sprite_cell;
};

class RenderSystem final : public System {
    ObjectData *const m_object_data;

public:
    explicit RenderSystem(ObjectData *object_data) : m_object_data(object_data) {}

    void update(World *world, float dt) override;
};

} // namespace v2d
