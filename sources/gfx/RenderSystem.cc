#include <v2d/gfx/RenderSystem.hh>

#include <v2d/core/Context.hh>
#include <v2d/core/Transform.hh>
#include <v2d/ecs/World.hh>
#include <v2d/gfx/Sprite.hh>
#include <v2d/maths/Common.hh>
#include <v2d/maths/Vec.hh>

namespace v2d {
namespace {

struct ObjectData {
    Vec2f position;
    Vec2f scale;
    Vec2f sprite_cell;
};

struct SceneData {
    Vec2f camera_position;
    ObjectData objects;
};

Vec2f s_camera_position;

} // namespace

void RenderSystem::update(World *world, float dt) {
    std::size_t object_count = 0;
    for (auto [entity, sprite] : world->view<Sprite>()) {
        V2D_ASSERT(entity.has<Transform>());
        object_count++;
    }

    const bool need_more_capacity = object_count > m_object_capacity;
    if (need_more_capacity || object_count < (m_object_capacity / 2)) {
        if (need_more_capacity) {
            m_object_capacity = v2d::max(m_object_capacity * 2 + 1, object_count);
        } else {
            m_object_capacity = v2d::max(object_count, 1ul);
        }
        m_object_buffer = m_context.create_buffer(m_object_capacity * sizeof(ObjectData) + sizeof(Vec2f),
                                                  BufferType::StorageBuffer, MemoryType::CpuVisible);
        VkDescriptorBufferInfo buffer_info{
            .buffer = *m_object_buffer,
            .range = VK_WHOLE_SIZE,
        };
        VkWriteDescriptorSet descriptor_write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_descriptor_set,
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &buffer_info,
        };
        vkUpdateDescriptorSets(m_context.device(), 1, &descriptor_write, 0, nullptr);
    }

    auto *scene_data = m_object_buffer.map<SceneData>();

    Vec2f rect_half_extents = Vec2f(800.0f, 600.0f) / 4.0f;

    auto &player_position = Entity(0, world).get<Transform>().position();
    if (player_position.x() < s_camera_position.x() - rect_half_extents.x()) {
        s_camera_position -= Vec2f(dt * 200.0f, 0.0f);
    } else if (player_position.x() > s_camera_position.x() + rect_half_extents.x()) {
        s_camera_position += Vec2f(dt * 200.0f, 0.0f);
    }
    if (player_position.y() < s_camera_position.y() - rect_half_extents.y()) {
        s_camera_position -= Vec2f(0.0f, dt * 200.0f);
    } else if (player_position.y() > s_camera_position.y() + rect_half_extents.y()) {
        s_camera_position += Vec2f(0.0f, dt * 200.0f);
    }

    scene_data->camera_position = s_camera_position / Vec2f(800.0f, 600.0f);
    for (std::size_t i = 0; auto [entity, sprite] : world->view<Sprite>()) {
        const auto &transform = entity.get<Transform>();
        auto &object_data = (&scene_data->objects)[i++];
        object_data.position = transform.position() / Vec2f(800.0f, 600.0f);
        object_data.scale = transform.scale() / Vec2f(800.0f, 600.0f);
        object_data.sprite_cell = {static_cast<float>(sprite->cell().x()), static_cast<float>(sprite->cell().y())};
    }
    m_object_buffer.unmap();
}

} // namespace v2d
