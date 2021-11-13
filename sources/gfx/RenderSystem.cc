#include <v2d/gfx/RenderSystem.hh>

#include <v2d/core/Transform.hh>
#include <v2d/ecs/World.hh>
#include <v2d/gfx/Sprite.hh>

namespace v2d {

void RenderSystem::update(World *world, float) {
    for (std::size_t i = 0; auto [entity, transform] : world->view<Transform>()) {
        auto &object_data = m_object_data[i++];
        object_data.position = transform->position();
        object_data.scale = transform->scale();
    }
    for (std::size_t i = 0; auto [entity, sprite] : world->view<Sprite>()) {
        Vec2f cell(static_cast<float>(sprite->cell().x()), static_cast<float>(sprite->cell().y()));
        m_object_data[i++].sprite_cell = cell;
    }
}

} // namespace v2d
