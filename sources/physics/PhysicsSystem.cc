#include <v2d/physics/PhysicsSystem.hh>

#include "Contact.hh"

#include <v2d/core/Transform.hh>
#include <v2d/ecs/World.hh>
#include <v2d/maths/Common.hh>
#include <v2d/physics/BoxCollider.hh>
#include <v2d/physics/RigidBody.hh>

namespace v2d {

void PhysicsSystem::update(World *world, float dt) {
    for (auto [entity, body] : world->view<RigidBody>()) {
        if (body->m_mass == 0.0f) {
            continue;
        }
        constexpr Vec2f gravity(0.0f, 500.0f);
        body->m_in_contact = false;
        body->m_velocity += (body->m_force * body->m_inverse_mass + gravity) * dt;
    }
    Vector<Contact> contacts;
    for (auto [e1, c1, b1] : world->view<BoxCollider, RigidBody>()) {
        if (b1->m_mass == 0.0f) {
            continue;
        }
        for (auto [e2, c2, b2] : world->view<BoxCollider, RigidBody>()) {
            if (e1.id() == e2.id()) {
                continue;
            }
            auto &t1 = e1.get<Transform>();
            auto &t2 = e2.get<Transform>();
            float width = c1->size().x() + c2->size().x();
            float height = c1->size().y() + c2->size().y();
            float x_overlap = v2d::abs(t1.position().x() - t2.position().x());
            float y_overlap = v2d::abs(t1.position().y() - t2.position().y());
            if (x_overlap + 0.1f > width || y_overlap + 0.1f > height) {
                continue;
            }
            b1->m_in_contact = true;
            b2->m_in_contact = true;
            if (y_overlap - height > x_overlap - width) {
                const float penetration = height - y_overlap;
                if ((t1.position().y() - t2.position().y()) < 0) {
                    contacts.emplace(Vec2f(0.0f, -1.0f), penetration, b1, b2, &t1, &t2);
                } else {
                    contacts.emplace(Vec2f(0.0f, 1.0f), penetration, b1, b2, &t1, &t2);
                }
            } else {
                const float penetration = width - x_overlap;
                if ((t1.position().x() - t2.position().x()) < 0) {
                    contacts.emplace(Vec2f(-1.0f, 0.0f), penetration, b1, b2, &t1, &t2);
                } else {
                    contacts.emplace(Vec2f(1.0f, 0.0f), penetration, b1, b2, &t1, &t2);
                }
            }
        }
    }
    for (auto &contact : contacts) {
        contact.pre_solve();
    }
    for (unsigned i = 0; i < 10; i++) {
        for (auto &contact : contacts) {
            contact.solve();
        }
    }
    for (auto &contact : contacts) {
        contact.post_solve();
    }
    for (auto [entity, body] : world->view<RigidBody>()) {
        auto &transform = entity.get<Transform>();
        transform.position() += body->m_velocity * dt;
        body->m_force = {};
    }
}

} // namespace v2d
