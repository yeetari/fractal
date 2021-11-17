#pragma once

#include <v2d/ecs/System.hh>
#include <v2d/gfx/Buffer.hh>

namespace v2d {

class Context;

class RenderSystem final : public System {
    const Context &m_context;
    const VkDescriptorSet m_descriptor_set;
    v2d::Buffer m_object_buffer;
    std::size_t m_object_capacity{0};

public:
    RenderSystem(const Context &context, VkDescriptorSet descriptor_set)
        : m_context(context), m_descriptor_set(descriptor_set) {}

    void update(World *world, float dt) override;
};

} // namespace v2d
