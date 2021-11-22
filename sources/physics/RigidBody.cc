#include <v2d/physics/RigidBody.hh>

#include <v2d/maths/Common.hh>

namespace v2d {

void RigidBody::apply_force(const Vec2f &force) {
    m_force += force;
}

void RigidBody::apply_impulse(const Vec2f &impulse) {
    m_velocity += impulse * m_inverse_mass;
}

void RigidBody::clamp_horizontal_velocity(float limit) {
    m_velocity = {v2d::clamp(m_velocity.x(), -limit, limit), m_velocity.y()};
}

void RigidBody::set_friction(float friction) {
    m_friction = friction;
}

} // namespace v2d
