#pragma once

#include <v2d/support/Span.hh>
#include <v2d/support/Vector.hh>

#include <vulkan/vulkan_core.h>

namespace v2d {

class Context;
class Window;

class Swapchain {
    const Context &m_context;
    VkQueue m_present_queue{nullptr};
    VkSurfaceKHR m_surface{nullptr};
    VkSurfaceCapabilitiesKHR m_surface_capabilities{};
    VkSwapchainKHR m_swapchain{nullptr};
    Vector<VkImageView> m_image_views;

public:
    Swapchain(const Context &context, const Window &window);
    Swapchain(const Swapchain &) = delete;
    Swapchain(Swapchain &&) = delete;
    ~Swapchain();

    Swapchain &operator=(const Swapchain &) = delete;
    Swapchain &operator=(Swapchain &&) = delete;

    std::uint32_t acquire_next_image(VkSemaphore semaphore) const;
    void present(std::uint32_t image_index, Span<VkSemaphore> wait_semaphores) const;

    VkFormat surface_format() const { return VK_FORMAT_B8G8R8A8_SRGB; }
    const VkImageView &image_view(std::uint32_t index) const { return m_image_views[index]; }
};

} // namespace v2d
