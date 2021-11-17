#include <v2d/ecs/Entity.hh>

namespace v2d {

void Entity::destroy() {
    m_manager->destroy_entity(m_id);
}

Entity EntityManager::create_entity() {
    // TODO: Entity id recycling.
    m_count++;
    return {m_next_id++, this};
}

void EntityManager::destroy_entity(EntityId id) {
    m_count--;
    for (auto &set : m_component_sets) {
        if (set.as<std::byte>().contains(id)) {
            set.as<std::byte>().remove(id);
        }
    }
}

} // namespace v2d
