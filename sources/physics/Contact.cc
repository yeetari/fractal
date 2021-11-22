#include "Contact.hh"

#include <v2d/core/Transform.hh>
#include <v2d/maths/Common.hh>
#include <v2d/physics/RigidBody.hh>

namespace v2d {

void Contact::pre_solve() {
    m_effective_mass = 1.0f / (m_b1->m_inverse_mass + m_b2->m_inverse_mass);
    m_friction = v2d::sqrt(m_b1->m_friction * m_b2->m_friction);
    m_restitution = v2d::max(m_b1->m_restitution, m_b2->m_restitution);
    m_tangent = {m_normal.y(), -m_normal.x()};
}

void Contact::solve() {
    Vec2f relative_velocity = m_b1->m_velocity - m_b2->m_velocity;
    float velocity_projection = relative_velocity.dot(m_normal);

    // Contact impulse.
    float contact_impulse = v2d::max((-(m_restitution + 1.0f) * velocity_projection) * m_effective_mass, 0.0f);
    Vec2f normal_impulse = m_normal * contact_impulse;
    m_b1->m_velocity += normal_impulse * m_b1->m_inverse_mass;
    m_b2->m_velocity -= normal_impulse * m_b2->m_inverse_mass;

    // Friction.
    relative_velocity = m_b1->m_velocity - m_b2->m_velocity;
    float friction_impulse = -relative_velocity.dot(m_tangent) * m_effective_mass;
    float max_friction = contact_impulse * m_friction;
    friction_impulse = v2d::clamp(friction_impulse, -max_friction, max_friction);
    Vec2f tangent_impulse = m_tangent * friction_impulse;
    m_b1->m_velocity += tangent_impulse * m_b1->m_inverse_mass;
    m_b2->m_velocity -= tangent_impulse * m_b2->m_inverse_mass;
}

void Contact::post_solve() {
    constexpr float slop = 0.05f;
    constexpr float percent = 0.2f;
    Vec2f position_correction = m_normal * (v2d::max(m_penetration - slop, 0.0f) * m_effective_mass * percent);
    m_t1->position() += position_correction * m_b1->m_inverse_mass;
    m_t2->position() -= position_correction * m_b2->m_inverse_mass;
}

} // namespace v2d
