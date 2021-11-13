#pragma once

#include <v2d/support/Vector.hh>

#include <memory>

namespace v2d {

struct World;

struct System {
    System() = default;
    System(const System &) = delete;
    System(System &&) = delete;
    virtual ~System() = default;

    System &operator=(const System &) = delete;
    System &operator=(System &&) = delete;

    virtual void update(World *world, float dt) = 0;
};

class SystemManager {
protected:
    Vector<std::unique_ptr<System>> m_systems;

public:
    template <typename S, typename... Args>
    void add(Args &&...args) {
        m_systems.emplace(new S(std::forward<Args>(args)...));
    }
};

} // namespace v2d
