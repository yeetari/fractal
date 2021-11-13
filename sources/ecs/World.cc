#include <v2d/ecs/World.hh>

namespace v2d {

void World::update(float dt) {
    for (const auto &system : m_systems) {
        system->update(this, dt);
    }
}

} // namespace v2d
