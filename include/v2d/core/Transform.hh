#pragma once

#include <v2d/core/BuiltinComponents.hh>
#include <v2d/ecs/Component.hh>
#include <v2d/maths/Vec.hh>

namespace v2d {

class Transform {
    V2D_DECLARE_COMPONENT(BuiltinComponents::Transform);

private:
    Vec2f m_position;
    Vec2f m_scale{1.0f};

public:
    explicit Transform(const Vec2f &position) : m_position(position) {}

    void set_position(const Vec2f &position) { m_position = position; }
    void set_scale(const Vec2f &scale) { m_scale = scale; }

    const Vec2f &position() const { return m_position; }
    const Vec2f &scale() const { return m_scale; }
};

} // namespace v2d
