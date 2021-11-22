#pragma once

#include <v2d/maths/Vec.hh>

namespace v2d {

class RigidBody;
class Transform;

class Contact {
    const Vec2f m_normal;
    const float m_penetration;
    RigidBody *const m_b1;
    RigidBody *const m_b2;
    Transform *const m_t1;
    Transform *const m_t2;
    Vec2f m_tangent;
    float m_effective_mass{0.0f};
    float m_friction{0.0f};
    float m_restitution{0.0f};

public:
    Contact(const Vec2f &normal, float penetration, RigidBody *b1, RigidBody *b2, Transform *t1, Transform *t2)
        : m_normal(normal), m_penetration(penetration), m_b1(b1), m_b2(b2), m_t1(t1), m_t2(t2) {}

    void pre_solve();
    void solve();
    void post_solve();
};

} // namespace v2d
