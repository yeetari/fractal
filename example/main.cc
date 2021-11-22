#include <v2d/core/Context.hh>
#include <v2d/core/Transform.hh>
#include <v2d/core/Window.hh>
#include <v2d/ecs/World.hh>
#include <v2d/gfx/RenderSystem.hh>
#include <v2d/gfx/Sprite.hh>
#include <v2d/gfx/Swapchain.hh>
#include <v2d/maths/Vec.hh>
#include <v2d/physics/BoxCollider.hh>
#include <v2d/physics/PhysicsSystem.hh>
#include <v2d/physics/RigidBody.hh>
#include <v2d/support/Assert.hh>
#include <v2d/support/Optional.hh>
#include <v2d/support/Vector.hh>

#define STBI_ASSERT(x) V2D_ASSERT(x)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <vulkan/vulkan_core.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <random>

#define VK_CHECK(expr, msg)                                                                                            \
    if (VkResult result = (expr); result != VK_SUCCESS && result != VK_INCOMPLETE) {                                   \
        std::fprintf(stderr, "%s (%s != VK_SUCCESS)\n", msg, #expr);                                                   \
        std::exit(1);                                                                                                  \
    }

namespace {

VkShaderModule load_shader(VkDevice device, const char *path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    V2D_ENSURE(file);
    v2d::Vector<char> binary(file.tellg());
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
    v2d::Context context(v2d::Window::required_instance_extensions());
    v2d::Window window(800, 600);
    v2d::Swapchain swapchain(context, window);

    std::uint32_t atlas_width;
    std::uint32_t atlas_height;
    auto *atlas_texture = stbi_load("atlas.png", reinterpret_cast<int *>(&atlas_width),
                                    reinterpret_cast<int *>(&atlas_height), nullptr, STBI_rgb_alpha);
    V2D_ENSURE(atlas_texture != nullptr);

    auto atlas_staging_buffer = context.create_buffer(atlas_width * atlas_height * 4, v2d::BufferType::TransferSrc,
                                                      v2d::MemoryType::CpuVisible);
    auto *atlas_staging_data = atlas_staging_buffer.map_raw();
    std::memcpy(atlas_staging_data, atlas_texture, atlas_width * atlas_height * 4);
    stbi_image_free(atlas_texture);

    VkImageCreateInfo atlas_image_ci{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .extent = {atlas_width, atlas_height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VkImage atlas_image;
    VK_CHECK(vkCreateImage(context.device(), &atlas_image_ci, nullptr, &atlas_image), "Failed to create atlas image")

    VkMemoryRequirements atlas_image_requirements;
    vkGetImageMemoryRequirements(context.device(), atlas_image, &atlas_image_requirements);
    VkDeviceMemory atlas_image_memory = context.allocate_memory(atlas_image_requirements, v2d::MemoryType::GpuOnly);
    VK_CHECK(vkBindImageMemory(context.device(), atlas_image, atlas_image_memory, 0),
             "Failed to bind atlas image memory")

    VkCommandPool command_pool = nullptr;
    VkQueue queue = nullptr;
    for (std::uint32_t i = 0; i < context.queue_families().size(); i++) {
        const auto &family = context.queue_families()[i];
        if ((family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0u) {
            VkCommandPoolCreateInfo command_pool_ci{
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .queueFamilyIndex = i,
            };
            VK_CHECK(vkCreateCommandPool(context.device(), &command_pool_ci, nullptr, &command_pool),
                     "Failed to create command pool")
            vkGetDeviceQueue(context.device(), i, 0, &queue);
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
    VK_CHECK(vkAllocateCommandBuffers(context.device(), &command_buffer_ai, &command_buffer),
             "Failed to allocate command buffer")

    VkImageMemoryBarrier atlas_transition_barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image = atlas_image,
        .subresourceRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        },
    };
    VkCommandBufferBeginInfo transfer_buffer_bi{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(command_buffer, &transfer_buffer_bi);
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &atlas_transition_barrier);
    VkBufferImageCopy atlas_copy{
        .imageSubresource{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1,
        },
        .imageExtent = {atlas_width, atlas_height, 1},
    };
    vkCmdCopyBufferToImage(command_buffer, *atlas_staging_buffer, atlas_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &atlas_copy);
    atlas_transition_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    atlas_transition_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    atlas_transition_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    atlas_transition_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &atlas_transition_barrier);
    vkEndCommandBuffer(command_buffer);
    VkSubmitInfo transfer_si{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
    };
    vkQueueSubmit(queue, 1, &transfer_si, nullptr);
    vkQueueWaitIdle(queue);

    VkImageViewCreateInfo atlas_image_view_ci{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = atlas_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = atlas_image_ci.format,
        .subresourceRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        },
    };
    VkImageView atlas_image_view = nullptr;
    VK_CHECK(vkCreateImageView(context.device(), &atlas_image_view_ci, nullptr, &atlas_image_view),
             "Failed to create atlas image view")

    VkSamplerCreateInfo atlas_sampler_ci{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    };
    VkSampler atlas_sampler = nullptr;
    VK_CHECK(vkCreateSampler(context.device(), &atlas_sampler_ci, nullptr, &atlas_sampler),
             "Failed to create atlas sampler")

    v2d::Array descriptor_pool_sizes{
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
        },
    };
    VkDescriptorPoolCreateInfo descriptor_pool_ci{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = descriptor_pool_sizes.size(),
        .pPoolSizes = descriptor_pool_sizes.data(),
    };
    VkDescriptorPool descriptor_pool = nullptr;
    VK_CHECK(vkCreateDescriptorPool(context.device(), &descriptor_pool_ci, nullptr, &descriptor_pool),
             "Failed to create descriptor pool")

    v2d::Array descriptor_bindings{
        // Atlas binding.
        VkDescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        // Object buffer binding.
        VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        },
    };
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_ci{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = descriptor_bindings.size(),
        .pBindings = descriptor_bindings.data(),
    };
    VkDescriptorSetLayout descriptor_set_layout = nullptr;
    VK_CHECK(vkCreateDescriptorSetLayout(context.device(), &descriptor_set_layout_ci, nullptr, &descriptor_set_layout),
             "Failed to create descriptor set layout")

    VkDescriptorSetAllocateInfo descriptor_set_ai{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptor_set_layout,
    };
    VkDescriptorSet descriptor_set = nullptr;
    VK_CHECK(vkAllocateDescriptorSets(context.device(), &descriptor_set_ai, &descriptor_set),
             "Failed to allocate descriptor set")

    VkDescriptorImageInfo atlas_image_info{
        .sampler = atlas_sampler,
        .imageView = atlas_image_view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    v2d::Array descriptor_writes{
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_set,
            .dstBinding = 1,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &atlas_image_info,
        },
    };
    vkUpdateDescriptorSets(context.device(), descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);

    VkAttachmentDescription attachment{
        .format = swapchain.surface_format(),
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
    VK_CHECK(vkCreateRenderPass(context.device(), &render_pass_ci, nullptr, &render_pass),
             "Failed to create render pass")

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
    VK_CHECK(vkCreateFramebuffer(context.device(), &framebuffer_ci, nullptr, &framebuffer),
             "Failed to create framebuffer")

    VkPipelineLayoutCreateInfo pipeline_layout_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptor_set_layout,
    };
    VkPipelineLayout pipeline_layout = nullptr;
    VK_CHECK(vkCreatePipelineLayout(context.device(), &pipeline_layout_ci, nullptr, &pipeline_layout),
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
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT, // NOLINT
    };
    VkPipelineColorBlendStateCreateInfo blend_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &blend_attachment,
    };

    auto *vertex_shader = load_shader(context.device(), "shaders/main.vert.spv");
    auto *fragment_shader = load_shader(context.device(), "shaders/main.frag.spv");
    v2d::Array shader_stage_cis{
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
    VK_CHECK(vkCreateGraphicsPipelines(context.device(), nullptr, 1, &pipeline_ci, nullptr, &pipeline),
             "Failed to create pipeline")

    VkFenceCreateInfo fence_ci{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    VkFence fence = nullptr;
    VK_CHECK(vkCreateFence(context.device(), &fence_ci, nullptr, &fence), "Failed to create fence")

    VkSemaphoreCreateInfo semaphore_ci{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    VkSemaphore image_available_semaphore = nullptr;
    VkSemaphore rendering_finished_semaphore = nullptr;
    VK_CHECK(vkCreateSemaphore(context.device(), &semaphore_ci, nullptr, &image_available_semaphore),
             "Failed to create semaphore")
    VK_CHECK(vkCreateSemaphore(context.device(), &semaphore_ci, nullptr, &rendering_finished_semaphore),
             "Failed to create semaphore")

    v2d::World world;
    world.add<v2d::PhysicsSystem>();
    world.add<v2d::RenderSystem>(context, descriptor_set);

    auto player = world.create_entity();
    player.add<v2d::Transform>(v2d::Vec2f(0.0f, 5.0f * 40.0f), v2d::Vec2f(42.0f));
    player.add<v2d::Sprite>(v2d::Vec2u(1u, 0u));
    player.add<v2d::BoxCollider>(v2d::Vec2f(15.0f, 21.0f));
    player.add<v2d::RigidBody>(1.0f, 0.05f);

    auto radial_start = world.create_entity();
    radial_start.add<v2d::Transform>(v2d::Vec2f(42.0f * 8.0f, 0.0f), v2d::Vec2f(21.0f));
    radial_start.add<v2d::Sprite>(v2d::Vec2u(2u, 0u));

    v2d::Vector<v2d::Entity> sliding_platforms;
    for (int i = 0; i < 5; i++) {
        auto platform = world.create_entity();
        platform.add<v2d::Transform>(v2d::Vec2f((21.0f + static_cast<float>(i)) * 42.0f, 0.0f), v2d::Vec2f(42.0f));
        platform.add<v2d::Sprite>(v2d::Vec2u(2u, 0u));
        platform.add<v2d::BoxCollider>(v2d::Vec2f(21.0f));
        platform.add<v2d::RigidBody>(0.0f, 0.0f);
        sliding_platforms.push(platform);
    }

    v2d::Vector<v2d::Entity> radial_platforms;
    for (int i = 0; i < 5; i++) {
        auto platform = world.create_entity();
        platform.add<v2d::Transform>(v2d::Vec2f(), v2d::Vec2f(42.0f));
        platform.add<v2d::Sprite>(v2d::Vec2u(2u, 0u));
        platform.add<v2d::BoxCollider>(v2d::Vec2f(21.0f));
        platform.add<v2d::RigidBody>(0.0f, 0.0f);
        radial_platforms.push(platform);
    }

    for (float x = 0; x < 4; x++) {
        auto ground_tile = world.create_entity();
        v2d::Vec2f position(x * 42.0f, 7.0f * 42.0f);
        ground_tile.add<v2d::Transform>(position, v2d::Vec2f(42.0f));
        ground_tile.add<v2d::Sprite>(v2d::Vec2u(0u, 0u));
        ground_tile.add<v2d::BoxCollider>(v2d::Vec2f(21.0f));
        ground_tile.add<v2d::RigidBody>(0.0f, 0.0f);
    }
    for (float x = 16; x < 21; x++) {
        auto ground_tile = world.create_entity();
        v2d::Vec2f position(x * 42.0f, 0.0f);
        ground_tile.add<v2d::Transform>(position, v2d::Vec2f(42.0f));
        ground_tile.add<v2d::Sprite>(v2d::Vec2u(0u, 0u));
        ground_tile.add<v2d::BoxCollider>(v2d::Vec2f(21.0f));
        ground_tile.add<v2d::RigidBody>(0.0f, 0.0f);
    }

    for (float x = 36; x < 41; x++) {
        auto ground_tile = world.create_entity();
        v2d::Vec2f position(x * 42.0f, 0.0f);
        ground_tile.add<v2d::Transform>(position, v2d::Vec2f(42.0f));
        ground_tile.add<v2d::Sprite>(v2d::Vec2u(0u, 0u));
        ground_tile.add<v2d::BoxCollider>(v2d::Vec2f(21.0f));
        ground_tile.add<v2d::RigidBody>(0.0f, 0.0f);
    }

    bool space_down = false;
    float count = 0.0f;

    auto previous_time = std::chrono::steady_clock::now();
    while (!window.should_close()) {
        auto current_time = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float, std::chrono::seconds::period>(current_time - previous_time).count();
        previous_time = current_time;
        std::uint32_t image_index = swapchain.acquire_next_image(image_available_semaphore);
        // TODO: Record command buffer here, before waiting for fence, instead?
        vkWaitForFences(context.device(), 1, &fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max());
        vkResetFences(context.device(), 1, &fence);
        world.update(dt);
        vkResetCommandPool(context.device(), command_pool, 0);
        VkCommandBufferBeginInfo cmd_buf_bi{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        vkBeginCommandBuffer(command_buffer, &cmd_buf_bi);

        VkClearValue clear_value{.color{{0.9f, 0.1f, 0.1f, 1.0f}}};
        VkRenderPassAttachmentBeginInfo attachment_bi{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO,
            .attachmentCount = 1,
            .pAttachments = &swapchain.image_view(image_index),
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
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set,
                                0, nullptr);
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        std::size_t object_count = 0;
        for (auto [entity, sprite] : world.view<v2d::Sprite>()) {
            object_count++;
        }
        vkCmdDraw(command_buffer, 6, object_count, 0, 0);
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

        auto &body = player.get<v2d::RigidBody>();
        float speed = body.in_contact() ? 15.0f : 1.0f;
        if (window.is_key_down(38)) {
            body.apply_impulse(v2d::Vec2f(-speed, 0.0f));
        } else if (window.is_key_down(40)) {
            body.apply_impulse(v2d::Vec2f(speed, 0.0f));
        }
        body.clamp_horizontal_velocity(200.0f);
        if (window.is_key_down(65) && !space_down && body.in_contact()) {
            space_down = true;
            body.apply_impulse(v2d::Vec2f(0.0f, -300.0f));
        } else if (!window.is_key_down(65)) {
            space_down = false;
        }

        for (auto &platform : sliding_platforms) {
            auto &platform_body = platform.get<v2d::RigidBody>();
            platform_body.set_velocity({std::sin(count) * 210.0f, 0.0f});
        }

        for (float x = -84.0f; auto &platform : radial_platforms) {
            auto &platform_body = platform.get<v2d::RigidBody>();
            auto &transform = platform.get<v2d::Transform>();
            v2d::Vec2f target_position(210.0f * std::sin(count / 2.0f), 294.0f * std::cos(count / 2.0f));
            target_position += radial_start.get<v2d::Transform>().position();
            target_position += v2d::Vec2f(x, 0.0f);
            platform_body.set_velocity((target_position - transform.position()) * 5.0f);
            x += 42.0f;
        }

        v2d::Array wait_semaphores{rendering_finished_semaphore};
        swapchain.present(image_index, wait_semaphores.span());
        window.poll_events();
        count += dt;
    }
    context.wait_idle();
    vkDestroySemaphore(context.device(), rendering_finished_semaphore, nullptr);
    vkDestroySemaphore(context.device(), image_available_semaphore, nullptr);
    vkDestroyFence(context.device(), fence, nullptr);
    vkDestroyPipeline(context.device(), pipeline, nullptr);
    vkDestroyShaderModule(context.device(), fragment_shader, nullptr);
    vkDestroyShaderModule(context.device(), vertex_shader, nullptr);
    vkDestroyPipelineLayout(context.device(), pipeline_layout, nullptr);
    vkDestroyFramebuffer(context.device(), framebuffer, nullptr);
    vkDestroyRenderPass(context.device(), render_pass, nullptr);
    vkDestroyDescriptorSetLayout(context.device(), descriptor_set_layout, nullptr);
    vkDestroyDescriptorPool(context.device(), descriptor_pool, nullptr);
    vkDestroySampler(context.device(), atlas_sampler, nullptr);
    vkDestroyImageView(context.device(), atlas_image_view, nullptr);
    vkDestroyCommandPool(context.device(), command_pool, nullptr);
    vkFreeMemory(context.device(), atlas_image_memory, nullptr);
    vkDestroyImage(context.device(), atlas_image, nullptr);
}
