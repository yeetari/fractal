#include <v2d/gfx/Buffer.hh>

#include <v2d/core/Context.hh>

namespace v2d {

Buffer::~Buffer() {
    vkFreeMemory(m_context->device(), m_memory, nullptr);
    vkDestroyBuffer(m_context->device(), m_buffer, nullptr);
}

void *Buffer::map_raw() const {
    void *data;
    vkMapMemory(m_context->device(), m_memory, 0, VK_WHOLE_SIZE, 0, &data);
    return data;
}

void Buffer::unmap() const {
    vkUnmapMemory(m_context->device(), m_memory);
}

} // namespace v2d
