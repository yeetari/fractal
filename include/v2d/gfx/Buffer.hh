#pragma once

#include <vulkan/vulkan_core.h>

#include <utility>

namespace v2d {

class Context;

enum class BufferType {
    StorageBuffer,
};

class Buffer {
    friend Context;

private:
    const Context *m_context{nullptr};
    VkBuffer m_buffer{nullptr};
    VkDeviceMemory m_memory{nullptr};

    Buffer(const Context *context, VkBuffer buffer, VkDeviceMemory memory)
        : m_context(context), m_buffer(buffer), m_memory(memory) {}

public:
    Buffer() = default;
    Buffer(const Buffer &) = delete;
    Buffer(Buffer &&other) noexcept
        : m_context(other.m_context), m_buffer(std::exchange(other.m_buffer, nullptr)),
          m_memory(std::exchange(other.m_memory, nullptr)) {}
    ~Buffer();

    Buffer &operator=(const Buffer &) = delete;
    Buffer &operator=(Buffer &&other) noexcept {
        Buffer buffer(std::move(other));
        m_context = buffer.m_context;
        std::swap(m_buffer, buffer.m_buffer);
        std::swap(m_memory, buffer.m_memory);
        return *this;
    }

    // TODO: RAII unmapping + map_raw returning TypeErased<BufferMapping>
    template <typename T>
    T *map() const { return static_cast<T *>(map_raw()); }
    void *map_raw() const;
    void unmap() const;

    VkBuffer operator*() const { return m_buffer; }
};

} // namespace v2d
