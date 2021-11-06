#pragma once

#include <v2d/support/Span.hh>
#include <v2d/support/Vector.hh>

#include <vulkan/vulkan_core.h>

namespace v2d {

class Context {
    VkInstance m_instance{nullptr};
    VkPhysicalDevice m_physical_device{nullptr};
    VkDevice m_device{nullptr};
    Vector<VkQueueFamilyProperties> m_queue_families;

public:
    explicit Context(Span<const char *const> extensions);
    Context(const Context &) = delete;
    Context(Context &&) = delete;
    ~Context();

    Context &operator=(const Context &) = delete;
    Context &operator=(Context &&) = delete;

    void wait_idle() const;

    VkInstance instance() const { return m_instance; }
    VkPhysicalDevice physical_device() const { return m_physical_device; }
    VkDevice device() const { return m_device; }
    const Vector<VkQueueFamilyProperties> &queue_families() const { return m_queue_families; }
};

} // namespace v2d
