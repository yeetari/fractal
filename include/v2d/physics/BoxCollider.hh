#pragma once

#include <v2d/core/BuiltinComponents.hh>
#include <v2d/ecs/Component.hh>
#include <v2d/maths/Vec.hh>

namespace v2d {

class BoxCollider {
    V2D_DECLARE_COMPONENT(BuiltinComponents::BoxCollider);

private:
    Vec2f m_size;

public:
    explicit BoxCollider(const Vec2f &size) : m_size(size) {}

    const Vec2f &size() const { return m_size; }
};

} // namespace v2d
