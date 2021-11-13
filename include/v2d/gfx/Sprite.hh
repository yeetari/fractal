#pragma once

#include <v2d/core/BuiltinComponents.hh>
#include <v2d/ecs/Component.hh>
#include <v2d/maths/Vec.hh>

namespace v2d {

class Sprite {
    V2D_DECLARE_COMPONENT(BuiltinComponents::Sprite);

private:
    Vec2u m_cell;

public:
    explicit Sprite(const Vec2u &cell) : m_cell(cell) {}

    const Vec2u &cell() const { return m_cell; }
};

} // namespace v2d
