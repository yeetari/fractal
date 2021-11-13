#pragma once

#include <v2d/support/SparseSet.hh>
#include <v2d/support/TypeErased.hh>

#include <array>
#include <cstddef>
#include <utility>

namespace v2d {

class EntityManager;
using EntityId = std::size_t;

class Entity {
    const EntityId m_id;
    EntityManager *const m_manager;

public:
    constexpr Entity(EntityId id, EntityManager *manager) : m_id(id), m_manager(manager) {}

    template <typename C, typename... Args>
    void add(Args &&...args);
    template <typename C>
    C &get();
    template <typename C>
    bool has() const;
    template <typename C, typename D, typename... Comps>
    bool has() const;
    template <typename C>
    void remove();

    void destroy();
    EntityId id() const { return m_id; }
};

template <typename C>
class EntitySingleIterator {
    EntityManager *const m_manager;
    std::pair<EntityId, C> *m_current;

public:
    EntitySingleIterator(EntityManager *manager, std::pair<EntityId, C> *current)
        : m_manager(manager), m_current(current) {}

    EntitySingleIterator &operator++() {
        m_current++;
        return *this;
    }
    auto operator<=>(const EntitySingleIterator &) const = default;
    std::pair<Entity, C *> operator*() const;
};

template <typename... Comps>
class EntityIterator {
    EntityManager *const m_manager;
    EntityId m_current;

public:
    EntityIterator(EntityManager *manager, EntityId current);

    EntityIterator &operator++();
    auto operator<=>(const EntityIterator &) const = default;
    std::tuple<Entity, Comps *...> operator*() const;
};

template <typename C>
class EntitySingleView {
    EntityManager *const m_manager;

public:
    EntitySingleView(EntityManager *manager) : m_manager(manager) {}

    EntitySingleIterator<C> begin() const;
    EntitySingleIterator<C> end() const;
};

template <typename... Comps>
class EntityView {
    EntityManager *const m_manager;

public:
    EntityView(EntityManager *manager) : m_manager(manager) {}

    EntityIterator<Comps...> begin() const;
    EntityIterator<Comps...> end() const;
};

class EntityManager {
    template <typename C>
    friend class EntitySingleView;

private:
    std::array<TypeErased<SparseSet, EntityId>, 16> m_component_sets;
    EntityId m_count{0};
    EntityId m_next_id{0};

    static std::size_t s_component_family_counter; // NOLINT

    template <typename>
    struct ComponentFamily {
        static std::size_t family() {
            static std::size_t family = s_component_family_counter++;
            return family;
        }
    };

public:
    template <typename C, typename... Args>
    void add_component(EntityId id, Args &&...args);
    template <typename C>
    C &get_component(EntityId id);
    template <typename C>
    bool has_component(EntityId id);
    template <typename C>
    void remove_component(EntityId id);

    Entity create_entity();
    void destroy_entity(EntityId id);

    template <typename C>
    EntitySingleView<C> view();
    template <typename C, typename D, typename... Comps>
    EntityView<C, D, Comps...> view();

    EntityId entity_count() const { return m_count; }
};

template <typename C, typename... Args>
void Entity::add(Args &&...args) {
    m_manager->add_component<C>(m_id, std::forward<Args>(args)...);
}

template <typename C>
C &Entity::get() {
    return m_manager->get_component<C>(m_id);
}

template <typename C>
bool Entity::has() const {
    return m_manager->has_component<C>(m_id);
}

template <typename C, typename D, typename... Comps>
bool Entity::has() const {
    return has<C>() && has<D, Comps...>();
}

template <typename C>
void Entity::remove() {
    m_manager->remove_component<C>(m_id);
}

template <typename C>
std::pair<Entity, C *> EntitySingleIterator<C>::operator*() const {
    return std::make_pair(Entity(m_current->first, m_manager), &m_current->second);
}

template <typename... Comps>
EntityIterator<Comps...>::EntityIterator(EntityManager *manager, EntityId current)
    : m_manager(manager), m_current(current) {
    while (m_current != m_manager->entity_count() && !Entity(m_current, m_manager).has<Comps...>()) {
        m_current++;
    }
}

template <typename... Comps>
EntityIterator<Comps...> &EntityIterator<Comps...>::operator++() {
    do {
        m_current++;
    } while (m_current != m_manager->entity_count() && !Entity(m_current, m_manager).has<Comps...>());
    return *this;
}

template <typename... Comps>
std::tuple<Entity, Comps *...> EntityIterator<Comps...>::operator*() const {
    return std::make_tuple(Entity(m_current, m_manager), &m_manager->get_component<Comps>(m_current)...);
}

template <typename C>
EntitySingleIterator<C> EntitySingleView<C>::begin() const {
    return {m_manager,
            m_manager->m_component_sets[EntityManager::ComponentFamily<C>::family()].template as<C>().begin()};
}

template <typename C>
EntitySingleIterator<C> EntitySingleView<C>::end() const {
    return {m_manager, m_manager->m_component_sets[EntityManager::ComponentFamily<C>::family()].template as<C>().end()};
}

template <typename... Comps>
EntityIterator<Comps...> EntityView<Comps...>::begin() const {
    return {m_manager, 0};
}

template <typename... Comps>
EntityIterator<Comps...> EntityView<Comps...>::end() const {
    return {m_manager, m_manager->entity_count()};
}

template <typename C, typename... Args>
void EntityManager::add_component(EntityId id, Args &&...args) {
    m_component_sets[ComponentFamily<C>::family()].template as<C>().insert(id, std::forward<Args>(args)...);
}

template <typename C>
C &EntityManager::get_component(EntityId id) {
    return m_component_sets[ComponentFamily<C>::family()].template as<C>()[id];
}

template <typename C>
bool EntityManager::has_component(EntityId id) {
    return m_component_sets[ComponentFamily<C>::family()].template as<C>().contains(id);
}

template <typename C>
void EntityManager::remove_component(EntityId id) {
    m_component_sets[ComponentFamily<C>::family()].template as<C>().remove(id);
}

template <typename C>
EntitySingleView<C> EntityManager::view() {
    return {this};
}

template <typename C, typename D, typename... Comps>
EntityView<C, D, Comps...> EntityManager::view() {
    return {this};
}

} // namespace v2d
