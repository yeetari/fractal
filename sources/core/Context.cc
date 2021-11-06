#include <v2d/core/Context.hh>

#include <v2d/support/Assert.hh>
#include <v2d/support/Vector.hh>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>

// Redefine the VK_MAKE_VERSION macro to make clang-tidy not complain about signed bitwise operations.
#undef VK_MAKE_VERSION
#define VK_MAKE_VERSION(major, minor, patch)                                                                           \
    ((static_cast<std::uint32_t>(major) << 22u) | (static_cast<std::uint32_t>(minor) << 12u) |                         \
     static_cast<std::uint32_t>(patch))

namespace v2d {

Context::Context(Span<const char *const> extensions) {
    VkApplicationInfo application_info{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "v2d",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = VK_MAKE_VERSION(1, 2, 0),
    };
    VkInstanceCreateInfo instance_ci{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &application_info,
        .enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };
#ifndef NDEBUG
    std::uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    Vector<VkLayerProperties> layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, layers.data());
    const char *validation_layer_name = "VK_LAYER_KHRONOS_validation";
    auto *it = std::find_if(layers.begin(), layers.end(), [validation_layer_name](const auto &layer) {
        return std::strcmp(static_cast<const char *>(layer.layerName), validation_layer_name) == 0;
    });
    if (it != layers.end()) {
        instance_ci.enabledLayerCount = 1;
        instance_ci.ppEnabledLayerNames = &validation_layer_name;
    } else {
        std::fputs("Validation layers not present!", stderr);
    }
#endif
    V2D_ENSURE(vkCreateInstance(&instance_ci, nullptr, &m_instance) == VK_SUCCESS);

    std::uint32_t physical_device_count = 1;
    VkResult enumeration_result = vkEnumeratePhysicalDevices(m_instance, &physical_device_count, &m_physical_device);
    V2D_ENSURE(enumeration_result == VK_SUCCESS || enumeration_result == VK_INCOMPLETE);

    std::uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_family_count, nullptr);
    m_queue_families.grow(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_family_count, m_queue_families.data());

    Vector<VkDeviceQueueCreateInfo> queue_cis;
    const float queue_priority = 1.0f;
    for (std::uint32_t i = 0; i < m_queue_families.size(); i++) {
        queue_cis.push({
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = i,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority,
        });
    }

    const char *swapchain_extension_name = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    VkPhysicalDeviceVulkan12Features device_12_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .imagelessFramebuffer = VK_TRUE,
    };
    VkDeviceCreateInfo device_ci{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &device_12_features,
        .queueCreateInfoCount = queue_cis.size(),
        .pQueueCreateInfos = queue_cis.data(),
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = &swapchain_extension_name,
    };
    V2D_ENSURE(vkCreateDevice(m_physical_device, &device_ci, nullptr, &m_device) == VK_SUCCESS);
}

Context::~Context() {
    vkDestroyDevice(m_device, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

void Context::wait_idle() const {
    vkDeviceWaitIdle(m_device);
}

} // namespace v2d
