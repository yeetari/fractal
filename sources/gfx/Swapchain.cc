#include <v2d/gfx/Swapchain.hh>

#include <v2d/core/Context.hh>
#include <v2d/core/Window.hh>
#include <v2d/support/Vector.hh>

#include <limits>

namespace v2d {

Swapchain::Swapchain(const Context &context, const Window &window) : m_context(context) {
    m_surface = window.create_surface(context.instance());
    for (std::uint32_t i = 0; i < context.queue_families().size(); i++) {
        VkBool32 present_supported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(context.physical_device(), i, m_surface, &present_supported);
        if (present_supported == VK_TRUE) {
            vkGetDeviceQueue(context.device(), i, 0, &m_present_queue);
            break;
        }
    }
    V2D_ENSURE(m_present_queue != nullptr, "Failed to find a suitable queue for presenting");

    VkSurfaceCapabilitiesKHR surface_capabilities{};
    V2D_ENSURE(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physical_device(), m_surface, &surface_capabilities) ==
               VK_SUCCESS);

    // TODO: Check surface format available.
    VkSurfaceFormatKHR surface_format{
        .format = VK_FORMAT_B8G8R8A8_SRGB,
        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    };
    VkSwapchainCreateInfoKHR swapchain_ci{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_surface,
        .minImageCount = surface_capabilities.minImageCount + 1,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = {window.width(), window.height()},
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = surface_capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
    };
    V2D_ENSURE(vkCreateSwapchainKHR(context.device(), &swapchain_ci, nullptr, &m_swapchain) == VK_SUCCESS);

    std::uint32_t image_count = 0;
    vkGetSwapchainImagesKHR(context.device(), m_swapchain, &image_count, nullptr);
    v2d::Vector<VkImage> images(image_count);
    m_image_views.ensure_size(image_count);
    vkGetSwapchainImagesKHR(context.device(), m_swapchain, &image_count, images.data());
    for (std::uint32_t i = 0; i < image_count; i++) {
        VkImageViewCreateInfo image_view_ci{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = surface_format.format,
            .components{
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };
        V2D_ENSURE(vkCreateImageView(m_context.device(), &image_view_ci, nullptr, &m_image_views[i]) == VK_SUCCESS);
    }
}

Swapchain::~Swapchain() {
    for (auto *image_view : m_image_views) {
        vkDestroyImageView(m_context.device(), image_view, nullptr);
    }
    vkDestroySwapchainKHR(m_context.device(), m_swapchain, nullptr);
    vkDestroySurfaceKHR(m_context.instance(), m_surface, nullptr);
}

std::uint32_t Swapchain::acquire_next_image(VkSemaphore semaphore) const {
    std::uint32_t image_index = 0;
    vkAcquireNextImageKHR(m_context.device(), m_swapchain, std::numeric_limits<std::uint64_t>::max(), semaphore,
                          nullptr, &image_index);
    return image_index;
}

void Swapchain::present(std::uint32_t image_index, Span<VkSemaphore> wait_semaphores) const {
    VkPresentInfoKHR present_info{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = wait_semaphores.size(),
        .pWaitSemaphores = wait_semaphores.data(),
        .swapchainCount = 1,
        .pSwapchains = &m_swapchain,
        .pImageIndices = &image_index,
    };
    vkQueuePresentKHR(m_present_queue, &present_info);
}

} // namespace v2d
