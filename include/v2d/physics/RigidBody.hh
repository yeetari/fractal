#pragma once

#include <v2d/core/BuiltinComponents.hh>
#include <v2d/ecs/Component.hh>
#include <v2d/maths/Vec.hh>

namespace v2d {

class RigidBody {
    friend class Contact;
    friend class PhysicsSystem;
    V2D_DECLARE_COMPONENT(BuiltinComponents::RigidBody);

private:
    const float m_mass;
    const float m_restitution;
    const float m_inverse_mass;
    float m_friction{0.4f};
    Vec2f m_force;
    Vec2f m_velocity;
    bool m_in_contact{false};

public:
    RigidBody(float mass, float restitution)
        : m_mass(mass), m_restitution(restitution), m_inverse_mass(mass != 0.0f ? 1.0f / mass : 0.0f) {}

    void apply_force(const Vec2f &force);
    void apply_impulse(const Vec2f &impulse);
    void clamp_horizontal_velocity(float limit);
    void set_friction(float friction);
    void set_velocity(const Vec2f &velocity) { m_velocity = velocity; }

    float mass() const { return m_mass; }
    float restitution() const { return m_restitution; }
    float inverse_mass() const { return m_inverse_mass; }
    float friction() const { return m_friction; }
    const Vec2f &velocity() const { return m_velocity; }
    bool in_contact() const { return m_in_contact; }
};

} // namespace v2d

constexpr float operator""_kg(long double mass) {
    return static_cast<float>(mass);
}

constexpr float operator""_t(long double mass) {
    return static_cast<float>(mass) * 1000.0f;
}
