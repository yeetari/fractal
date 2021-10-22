#include "Vector.hh"
#include "Window.hh"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <limits>

#define VK_CHECK(expr, msg)                                                                                            \
    if (VkResult result = (expr); result != VK_SUCCESS && result != VK_INCOMPLETE) {                                   \
        std::fprintf(stderr, "%s (%s != VK_SUCCESS)\n", msg, #expr);                                                   \
        std::exit(1);                                                                                                  \
    }

#undef VK_MAKE_VERSION
#define VK_MAKE_VERSION(major, minor, patch)                                                                           \
    ((static_cast<std::uint32_t>(major) << 22u) | (static_cast<std::uint32_t>(minor) << 12u) |                         \
     static_cast<std::uint32_t>(patch))

namespace {

VkShaderModule load_shader(VkDevice device, const char *path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    assert(file);
    Vector<char> binary(file.tellg());
    file.seekg(0);
    file.read(binary.data(), binary.size());
    VkShaderModuleCreateInfo module_ci{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = binary.size(),
        .pCode = reinterpret_cast<const std::uint32_t *>(binary.data()),
    };
    VkShaderModule module = nullptr;
    VK_CHECK(vkCreateShaderModule(device, &module_ci, nullptr, &module), "Failed to create shader module")
    return module;
}

} // namespace

int main() {
    Window window(800, 600, false);
    VkApplicationInfo application_info{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "fractal",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = VK_MAKE_VERSION(1, 2, 0),
    };
    const auto required_extensions = Window::required_instance_extensions();
    VkInstanceCreateInfo instance_ci{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &application_info,
        .enabledExtensionCount = static_cast<std::uint32_t>(required_extensions.size()),
        .ppEnabledExtensionNames = required_extensions.data(),
    };
#ifndef NDEBUG
    // TODO: Check for existence.
    const char *validation_layer_name = "VK_LAYER_KHRONOS_validation";
    instance_ci.enabledLayerCount = 1;
    instance_ci.ppEnabledLayerNames = &validation_layer_name;
#endif
    VkInstance instance = nullptr;
    VK_CHECK(vkCreateInstance(&instance_ci, nullptr, &instance), "Failed to create instance")

    VkPhysicalDevice physical_device = nullptr;
    std::uint32_t physical_device_count = 1;
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &physical_device_count, &physical_device),
             "Failed to get physical device")

    std::uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
    Vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

    Vector<VkDeviceQueueCreateInfo> queue_cis;
    const float queue_priority = 1.0f;
    for (std::uint32_t i = 0; i < queue_families.size(); i++) {
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
    VkDevice device = nullptr;
    VK_CHECK(vkCreateDevice(physical_device, &device_ci, nullptr, &device), "Failed to create device")

    VkSurfaceKHR surface = nullptr;
    VK_CHECK(glfwCreateWindowSurface(instance, *window, nullptr, &surface), "Failed to create surface")

    VkSurfaceCapabilitiesKHR surface_capabilities{};
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities),
             "Failed to get surface capabilities")

    VkQueue present_queue = nullptr;
    for (std::uint32_t i = 0; i < queue_families.size(); i++) {
        VkBool32 present_supported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_supported);
        if (present_supported == VK_TRUE) {
            vkGetDeviceQueue(device, i, 0, &present_queue);
            break;
        }
    }

    VkSurfaceFormatKHR surface_format{
        .format = VK_FORMAT_B8G8R8A8_SRGB,
        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    };
    VkSwapchainCreateInfoKHR swapchain_ci{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
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
    VkSwapchainKHR swapchain = nullptr;
    VK_CHECK(vkCreateSwapchainKHR(device, &swapchain_ci, nullptr, &swapchain), "Failed to create swapchain")

    std::uint32_t swapchain_image_count = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr);
    Vector<VkImage> swapchain_images(swapchain_image_count);
    Vector<VkImageView> swapchain_image_views(swapchain_image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images.data());
    for (std::uint32_t i = 0; i < swapchain_image_count; i++) {
        VkImageViewCreateInfo image_view_ci{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchain_images[i],
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
        VK_CHECK(vkCreateImageView(device, &image_view_ci, nullptr, &swapchain_image_views[i]),
                 "Failed to create swapchain image view")
    }

    VkCommandPool command_pool = nullptr;
    VkQueue queue = nullptr;
    for (std::uint32_t i = 0; i < queue_families.size(); i++) {
        const auto &family = queue_families[i];
        if ((family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0u) {
            VkCommandPoolCreateInfo command_pool_ci{
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .queueFamilyIndex = i,
            };
            VK_CHECK(vkCreateCommandPool(device, &command_pool_ci, nullptr, &command_pool),
                     "Failed to create command pool")
            vkGetDeviceQueue(device, i, 0, &queue);
            break;
        }
    }

    VkCommandBufferAllocateInfo command_buffer_ai{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer command_buffer = nullptr;
    VK_CHECK(vkAllocateCommandBuffers(device, &command_buffer_ai, &command_buffer), "Failed to allocate command buffer")

    VkAttachmentDescription attachment{
        .format = surface_format.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    VkAttachmentReference attachment_reference{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    VkSubpassDescription subpass{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachment_reference,
    };
    VkRenderPassCreateInfo render_pass_ci{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };
    VkRenderPass render_pass = nullptr;
    VK_CHECK(vkCreateRenderPass(device, &render_pass_ci, nullptr, &render_pass), "Failed to create render pass")

    VkFramebufferAttachmentImageInfo attachment_info{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .width = window.width(),
        .height = window.height(),
        .layerCount = 1,
        .viewFormatCount = 1,
        .pViewFormats = &attachment.format,
    };
    VkFramebufferAttachmentsCreateInfo attachments_ci{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO,
        .attachmentImageInfoCount = 1,
        .pAttachmentImageInfos = &attachment_info,
    };
    VkFramebufferCreateInfo framebuffer_ci{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = &attachments_ci,
        .flags = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT,
        .renderPass = render_pass,
        .attachmentCount = 1,
        .width = window.width(),
        .height = window.height(),
        .layers = 1,
    };
    VkFramebuffer framebuffer = nullptr;
    VK_CHECK(vkCreateFramebuffer(device, &framebuffer_ci, nullptr, &framebuffer), "Failed to create framebuffer")

    VkPipelineLayoutCreateInfo pipeline_layout_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    };
    VkPipelineLayout pipeline_layout = nullptr;
    VK_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_ci, nullptr, &pipeline_layout),
             "Failed to create pipeline layout")

    VkPipelineVertexInputStateCreateInfo vertex_input{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };
    VkPipelineInputAssemblyStateCreateInfo input_assembly{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    VkRect2D scissor{
        .extent{window.width(), window.height()},
    };
    VkViewport viewport{
        .width = static_cast<float>(window.width()),
        .height = static_cast<float>(window.height()),
        .maxDepth = 1.0f,
    };
    VkPipelineViewportStateCreateInfo viewport_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    VkPipelineRasterizationStateCreateInfo rasterisation_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisample_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0f,
    };

    VkPipelineColorBlendAttachmentState blend_attachment{
        // NOLINTNEXTLINE
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT, // NOLINT
    };
    VkPipelineColorBlendStateCreateInfo blend_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &blend_attachment,
    };

    auto *vertex_shader = load_shader(device, "main.vert.spv");
    auto *fragment_shader = load_shader(device, "main.frag.spv");
    std::array shader_stage_cis{
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertex_shader,
            .pName = "main",
        },
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragment_shader,
            .pName = "main",
        },
    };
    VkGraphicsPipelineCreateInfo pipeline_ci{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = shader_stage_cis.size(),
        .pStages = shader_stage_cis.data(),
        .pVertexInputState = &vertex_input,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterisation_state,
        .pMultisampleState = &multisample_state,
        .pColorBlendState = &blend_state,
        .layout = pipeline_layout,
        .renderPass = render_pass,
    };
    VkPipeline pipeline = nullptr;
    VK_CHECK(vkCreateGraphicsPipelines(device, nullptr, 1, &pipeline_ci, nullptr, &pipeline),
             "Failed to create pipeline")

    VkFenceCreateInfo fence_ci{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    VkFence fence = nullptr;
    VK_CHECK(vkCreateFence(device, &fence_ci, nullptr, &fence), "Failed to create fence")

    VkSemaphoreCreateInfo semaphore_ci{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    VkSemaphore image_available_semaphore = nullptr;
    VkSemaphore rendering_finished_semaphore = nullptr;
    VK_CHECK(vkCreateSemaphore(device, &semaphore_ci, nullptr, &image_available_semaphore),
             "Failed to create semaphore")
    VK_CHECK(vkCreateSemaphore(device, &semaphore_ci, nullptr, &rendering_finished_semaphore),
             "Failed to create semaphore")

    while (!window.should_close()) {
        std::uint32_t image_index = 0;
        vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<std::uint64_t>::max(), image_available_semaphore,
                              nullptr, &image_index);
        // TODO: Record command buffer here, before waiting for fence, instead?
        vkWaitForFences(device, 1, &fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max());
        vkResetFences(device, 1, &fence);

        vkResetCommandPool(device, command_pool, 0);
        VkCommandBufferBeginInfo cmd_buf_bi{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        vkBeginCommandBuffer(command_buffer, &cmd_buf_bi);

        VkClearValue clear_value{.color{{0.0f, 0.0f, 0.0f, 1.0f}}};
        VkRenderPassAttachmentBeginInfo attachment_bi{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO,
            .attachmentCount = 1,
            .pAttachments = &swapchain_image_views[image_index],
        };
        VkRenderPassBeginInfo render_pass_bi{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = &attachment_bi,
            .renderPass = render_pass,
            .framebuffer = framebuffer,
            .renderArea{.extent{window.width(), window.height()}},
            .clearValueCount = 1,
            .pClearValues = &clear_value,
        };
        vkCmdBeginRenderPass(command_buffer, &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdDraw(command_buffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(command_buffer);
        vkEndCommandBuffer(command_buffer);

        VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit_info{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &image_available_semaphore,
            .pWaitDstStageMask = &wait_stage_mask,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &rendering_finished_semaphore,
        };
        vkQueueSubmit(queue, 1, &submit_info, fence);

        VkPresentInfoKHR present_info{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &rendering_finished_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pImageIndices = &image_index,
        };
        vkQueuePresentKHR(present_queue, &present_info);
        Window::poll_events();
    }
    vkDeviceWaitIdle(device);
    vkDestroySemaphore(device, rendering_finished_semaphore, nullptr);
    vkDestroySemaphore(device, image_available_semaphore, nullptr);
    vkDestroyFence(device, fence, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyShaderModule(device, fragment_shader, nullptr);
    vkDestroyShaderModule(device, vertex_shader, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyFramebuffer(device, framebuffer, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);
    vkDestroyCommandPool(device, command_pool, nullptr);
    for (auto *image_view : swapchain_image_views) {
        vkDestroyImageView(device, image_view, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
}
