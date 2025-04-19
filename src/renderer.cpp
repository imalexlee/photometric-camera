#include "renderer.h"
#include "loader.h"

#include <camera.h>
#include <functional>

static void vk_command_immediate_submit(VkDevice device, VkCommandPool command_pool, VkQueue queue,
                                        std::function<void(VkCommandBuffer command_buffer)>&& function) {

    VkFence                 fence{};
    const VkFenceCreateInfo fence_ci = vk_lib::fence_create_info();
    VK_CHECK(vkCreateFence(device, &fence_ci, nullptr, &fence));

    const VkCommandBufferAllocateInfo command_buffer_ai = vk_lib::command_buffer_allocate_info(command_pool);
    VkCommandBuffer                   cmd_buf;
    VK_CHECK(vkAllocateCommandBuffers(device, &command_buffer_ai, &cmd_buf));

    const VkCommandBufferBeginInfo command_buffer_bi = vk_lib::command_buffer_begin_info();
    VK_CHECK(vkBeginCommandBuffer(cmd_buf, &command_buffer_bi));

    function(cmd_buf);

    VK_CHECK(vkEndCommandBuffer(cmd_buf));

    const VkCommandBufferSubmitInfo command_buffer_submit_info = vk_lib::command_buffer_submit_info(cmd_buf);
    const VkSubmitInfo2             submit_info_2              = vk_lib::submit_info_2(&command_buffer_submit_info);

    VK_CHECK(vkQueueSubmit2(queue, 1, &submit_info_2, fence));

    VK_CHECK(vkWaitForFences(device, 1, &fence, true, UINT64_MAX));

    vkFreeCommandBuffers(device, command_pool, 1, &cmd_buf);

    vkDestroyFence(device, fence, nullptr);
}

static VkShaderModule load_shader(VkDevice device, const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        abort_message("Failed to find shader");
    }
    const size_t      file_size = file.tellg();
    std::vector<char> shader_data(file_size);
    file.seekg(0);
    file.read(shader_data.data(), static_cast<uint32_t>(file_size));
    VkShaderModule           shader_module;
    VkShaderModuleCreateInfo shader_module_ci = vk_lib::shader_module_create_info(reinterpret_cast<const uint32_t*>(shader_data.data()), file_size);
    VK_CHECK(vkCreateShaderModule(device, &shader_module_ci, nullptr, &shader_module));
    return shader_module;
}

static void renderer_create_graphics_pipeline(Renderer* renderer, VkDevice device, VkFormat color_attachment_format) {

    std::array                 set_layouts          = {renderer->scene_descriptor_set_layout, renderer->asset_descriptor_set_layout};
    VkPushConstantRange        push_constant_range  = vk_lib::push_constant_range(VK_SHADER_STAGE_ALL, sizeof(PushConstants));
    std::array                 push_constant_ranges = {push_constant_range};
    VkPipelineLayoutCreateInfo layout_create_info   = vk_lib::pipeline_layout_create_info(set_layouts, push_constant_ranges);
    VkPipelineLayout           pipeline_layout;
    vkCreatePipelineLayout(device, &layout_create_info, nullptr, &pipeline_layout);

    std::array                             color_attachment_formats = {color_attachment_format};
    const VkPipelineRenderingCreateInfoKHR rendering_create_info =
        vk_lib::pipeline_rendering_create_info(color_attachment_formats, VK_FORMAT_D32_SFLOAT);

    VkShaderModule                       vert_shader        = load_shader(device, "../shaders/indexed_draw.vert.spv");
    VkShaderModule                       frag_shader        = load_shader(device, "../shaders/gltf_pbr.frag.spv");
    VkPipelineShaderStageCreateInfo      vert_shader_stage  = vk_lib::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vert_shader);
    VkPipelineShaderStageCreateInfo      frag_shader_stage  = vk_lib::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader);
    std::array                           shader_stages      = {vert_shader_stage, frag_shader_stage};
    VkPipelineVertexInputStateCreateInfo vertex_input_state = vk_lib::pipeline_vertex_input_state_create_info();
    // todo: make input assembly state dynamic later on
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
        vk_lib::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    VkPipelineViewportStateCreateInfo      viewport_state = vk_lib::pipeline_viewport_state_create_info(nullptr, nullptr);
    VkPipelineRasterizationStateCreateInfo rasterization_state =
        vk_lib::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_CLOCKWISE);
    VkPipelineMultisampleStateCreateInfo  multisample_state                   = vk_lib::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_4_BIT);
    VkPipelineColorBlendAttachmentState   opaque_color_blend_attachment_state = vk_lib::pipeline_color_blend_attachment_state();
    std::array                            opaque_color_blends                 = {opaque_color_blend_attachment_state};
    VkPipelineColorBlendStateCreateInfo   opaque_color_blend_state            = vk_lib::pipeline_color_blend_state_create_info(opaque_color_blends);
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = vk_lib::pipeline_depth_stencil_state_create_info(true, true, VK_COMPARE_OP_GREATER);
    std::array                            dynamic_state_types = {VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT};
    VkPipelineDynamicStateCreateInfo      dynamic_state       = vk_lib::pipeline_dynamic_state_create_info(dynamic_state_types);
    VkGraphicsPipelineCreateInfo          opaque_graphics_pipeline_ci = vk_lib::graphics_pipeline_create_info(
        pipeline_layout, nullptr, shader_stages, &vertex_input_state, &input_assembly_state, &viewport_state, &rasterization_state,
        &multisample_state, &opaque_color_blend_state, &depth_stencil_state, &dynamic_state, nullptr, 0, 0, nullptr, 0, &rendering_create_info);

    VkPipeline opaque_pipeline;
    vkCreateGraphicsPipelines(device, nullptr, 1, &opaque_graphics_pipeline_ci, nullptr, &opaque_pipeline);

    GraphicsPipeline opaque_graphics_pipeline{};
    opaque_graphics_pipeline.pipeline        = opaque_pipeline;
    opaque_graphics_pipeline.pipeline_layout = pipeline_layout;
    opaque_graphics_pipeline.vert_shader     = vert_shader;
    opaque_graphics_pipeline.frag_shader     = frag_shader;

    renderer->opaque_graphics_pipeline = opaque_graphics_pipeline;

    VkPipelineColorBlendAttachmentState transparent_color_blend_attachment_state = vk_lib::pipeline_color_blend_attachment_state();
    std::array                          transparent_color_blends                 = {transparent_color_blend_attachment_state};
    VkPipelineColorBlendStateCreateInfo transparent_color_blend_state = vk_lib::pipeline_color_blend_state_create_info(transparent_color_blends);

    VkGraphicsPipelineCreateInfo transparent_graphics_pipeline_ci = vk_lib::graphics_pipeline_create_info(
        pipeline_layout, nullptr, shader_stages, &vertex_input_state, &input_assembly_state, &viewport_state, &rasterization_state,
        &multisample_state, &transparent_color_blend_state, &depth_stencil_state, &dynamic_state, nullptr, 0, 0, nullptr, 0, &rendering_create_info);

    VkPipeline transparent_pipeline;
    vkCreateGraphicsPipelines(device, nullptr, 1, &transparent_graphics_pipeline_ci, nullptr, &transparent_pipeline);

    GraphicsPipeline transparent_graphics_pipeline{};
    transparent_graphics_pipeline.pipeline        = transparent_pipeline;
    transparent_graphics_pipeline.pipeline_layout = pipeline_layout;
    transparent_graphics_pipeline.vert_shader     = vert_shader;
    transparent_graphics_pipeline.frag_shader     = frag_shader;

    renderer->transparent_graphics_pipeline = transparent_graphics_pipeline;
}

static VmaAllocator allocator_create(const VkContext* vk_context) {
    // since I'm using volk, have to specify function pointers for vma
    VmaVulkanFunctions vma_vulkan_functions{};
    vma_vulkan_functions.vkAllocateMemory                        = vkAllocateMemory;
    vma_vulkan_functions.vkBindBufferMemory                      = vkBindBufferMemory;
    vma_vulkan_functions.vkBindImageMemory                       = vkBindImageMemory;
    vma_vulkan_functions.vkCreateBuffer                          = vkCreateBuffer;
    vma_vulkan_functions.vkCreateImage                           = vkCreateImage;
    vma_vulkan_functions.vkDestroyBuffer                         = vkDestroyBuffer;
    vma_vulkan_functions.vkDestroyImage                          = vkDestroyImage;
    vma_vulkan_functions.vkFlushMappedMemoryRanges               = vkFlushMappedMemoryRanges;
    vma_vulkan_functions.vkFreeMemory                            = vkFreeMemory;
    vma_vulkan_functions.vkGetBufferMemoryRequirements           = vkGetBufferMemoryRequirements;
    vma_vulkan_functions.vkGetImageMemoryRequirements            = vkGetImageMemoryRequirements;
    vma_vulkan_functions.vkGetPhysicalDeviceMemoryProperties     = vkGetPhysicalDeviceMemoryProperties;
    vma_vulkan_functions.vkGetPhysicalDeviceProperties           = vkGetPhysicalDeviceProperties;
    vma_vulkan_functions.vkInvalidateMappedMemoryRanges          = vkInvalidateMappedMemoryRanges;
    vma_vulkan_functions.vkMapMemory                             = vkMapMemory;
    vma_vulkan_functions.vkUnmapMemory                           = vkUnmapMemory;
    vma_vulkan_functions.vkCmdCopyBuffer                         = vkCmdCopyBuffer;
    vma_vulkan_functions.vkGetInstanceProcAddr                   = vkGetInstanceProcAddr;
    vma_vulkan_functions.vkGetDeviceProcAddr                     = vkGetDeviceProcAddr;
    vma_vulkan_functions.vkGetBufferMemoryRequirements2KHR       = vkGetBufferMemoryRequirements2KHR;
    vma_vulkan_functions.vkGetImageMemoryRequirements2KHR        = vkGetImageMemoryRequirements2KHR;
    vma_vulkan_functions.vkBindBufferMemory2KHR                  = vkBindBufferMemory2KHR;
    vma_vulkan_functions.vkBindImageMemory2KHR                   = vkBindImageMemory2KHR;
    vma_vulkan_functions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR;
    vma_vulkan_functions.vkGetDeviceBufferMemoryRequirements     = vkGetDeviceBufferMemoryRequirements;
    vma_vulkan_functions.vkGetDeviceImageMemoryRequirements      = vkGetDeviceImageMemoryRequirements;

    VmaAllocatorCreateInfo allocator_create_info = {};
    allocator_create_info.vulkanApiVersion       = VK_API_VERSION_1_3;
    allocator_create_info.physicalDevice         = vk_context->physical_device;
    allocator_create_info.device                 = vk_context->device;
    allocator_create_info.instance               = vk_context->instance;
    allocator_create_info.pVulkanFunctions       = &vma_vulkan_functions;
    allocator_create_info.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    VmaAllocator allocator;
    VK_CHECK(vmaCreateAllocator(&allocator_create_info, &allocator));

    return allocator;
}

static void create_render_resources(Renderer* renderer) {
    VkContext*              vk_ctx        = &renderer->vk_context;
    const SwapchainContext* swapchain_ctx = &renderer->swapchain_context;

    VkExtent3D image_extent{};
    image_extent.width  = swapchain_ctx->extent.width;
    image_extent.height = swapchain_ctx->extent.height;
    image_extent.depth  = 1;

    // create main msaa color image
    VkImageCreateInfo msaa_image_ci =
        vk_lib::image_create_info(swapchain_ctx->surface_format.format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                  image_extent, 1, 1, VK_SAMPLE_COUNT_4_BIT);

    VmaAllocationCreateInfo allocation_ci{};
    allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    VK_CHECK(vmaCreateImage(renderer->allocator, &msaa_image_ci, &allocation_ci, &renderer->msaa_color_image.image,
                            &renderer->msaa_color_image.allocation, &renderer->msaa_color_image.allocation_info));

    renderer->msaa_color_image.image_format = swapchain_ctx->surface_format.format;

    VkImageSubresourceRange msaa_subresource_range = vk_lib::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

    VkImageViewCreateInfo msaa_image_view_ci =
        vk_lib::image_view_create_info(swapchain_ctx->surface_format.format, renderer->msaa_color_image.image, &msaa_subresource_range);
    vkCreateImageView(vk_ctx->device, &msaa_image_view_ci, nullptr, &renderer->msaa_color_image.image_view);

    // create depth image for the msaa color image. requires sample sample count
    VkImageCreateInfo depth_image_ci =
        vk_lib::image_create_info(VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, image_extent, 1, 1, VK_SAMPLE_COUNT_4_BIT);

    VK_CHECK(vmaCreateImage(renderer->allocator, &depth_image_ci, &allocation_ci, &renderer->depth_image.image, &renderer->depth_image.allocation,
                            &renderer->depth_image.allocation_info));

    renderer->depth_image.image_format = VK_FORMAT_D32_SFLOAT;

    VkImageSubresourceRange depth_subresource_range = vk_lib::image_subresource_range(VK_IMAGE_ASPECT_DEPTH_BIT);

    VkImageViewCreateInfo depth_image_view_ci =
        vk_lib::image_view_create_info(VK_FORMAT_D32_SFLOAT, renderer->depth_image.image, &depth_subresource_range);

    vkCreateImageView(vk_ctx->device, &depth_image_view_ci, nullptr, &renderer->depth_image.image_view);
}

static void destroy_render_resources(Renderer* renderer) {
    vmaDestroyImage(renderer->allocator, renderer->msaa_color_image.image, renderer->msaa_color_image.allocation);
    vkDestroyImageView(renderer->vk_context.device, renderer->msaa_color_image.image_view, nullptr);

    vmaDestroyImage(renderer->allocator, renderer->depth_image.image, renderer->depth_image.allocation);
    vkDestroyImageView(renderer->vk_context.device, renderer->depth_image.image_view, nullptr);
}

static void renderer_add_materials(Renderer* renderer, std::span<Material> materials) {
    uint64_t           new_material_alloc_size = renderer->material_buffer.allocation_info.size + materials.size() * sizeof(Material);
    VkBufferCreateInfo material_buf_ci         = vk_lib::buffer_create_info(
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, new_material_alloc_size);
    VmaAllocationCreateInfo material_buf_allocation_ci{};
    material_buf_allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    AllocatedBuffer new_material_buffer{};
    VK_CHECK(vmaCreateBuffer(renderer->allocator, &material_buf_ci, &material_buf_allocation_ci, &new_material_buffer.buffer,
                             &new_material_buffer.allocation, &new_material_buffer.allocation_info));

    VkBufferCreateInfo      staging_buf_ci = vk_lib::buffer_create_info(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, materials.size_bytes());
    VmaAllocationCreateInfo staging_buf_allocation_ci{};
    staging_buf_allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    staging_buf_allocation_ci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    AllocatedBuffer staging_buffer;
    VK_CHECK(vmaCreateBuffer(renderer->allocator, &staging_buf_ci, &staging_buf_allocation_ci, &staging_buffer.buffer, &staging_buffer.allocation,
                             &staging_buffer.allocation_info));

    memcpy(staging_buffer.allocation_info.pMappedData, materials.data(), materials.size_bytes());

    if (renderer->material_buffer.allocation_info.size > 0) {
        vk_command_immediate_submit(
            renderer->vk_context.device, renderer->vk_context.frame_command_pool, renderer->vk_context.graphics_queue, [&](VkCommandBuffer cmd_buf) {
                VkBufferCopy old_buffer_copy = vk_lib::buffer_copy(sizeof(Material) * renderer->material_count);
                vkCmdCopyBuffer(cmd_buf, renderer->material_buffer.buffer, new_material_buffer.buffer, 1, &old_buffer_copy);
                // may not need any memory barriers since copies happen at different buffer locations to new_material_buffer
                // VkBufferMemoryBarrier2 buffer_barrier = vk_lib::VkDependencyInfo dependency_info =
                //     vk_lib::dependency_info(nullptr, &buffer_barrier, nullptr);
                // vkCmdPipelineBarrier2(cmd_buf, &dependency_info);

                VkBufferCopy staging_buffer_copy = vk_lib::buffer_copy(materials.size_bytes(), 0, sizeof(Material) * renderer->material_count);
                vkCmdCopyBuffer(cmd_buf, staging_buffer.buffer, new_material_buffer.buffer, 1, &staging_buffer_copy);
            });
        vmaDestroyBuffer(renderer->allocator, renderer->material_buffer.buffer, renderer->material_buffer.allocation);
    } else {
        vk_command_immediate_submit(renderer->vk_context.device, renderer->vk_context.frame_command_pool, renderer->vk_context.graphics_queue,
                                    [&](VkCommandBuffer cmd_buf) {
                                        VkBufferCopy staging_buffer_copy = vk_lib::buffer_copy(materials.size_bytes());
                                        vkCmdCopyBuffer(cmd_buf, staging_buffer.buffer, new_material_buffer.buffer, 1, &staging_buffer_copy);
                                    });
    }
    vmaDestroyBuffer(renderer->allocator, staging_buffer.buffer, staging_buffer.allocation);

    renderer->material_buffer = new_material_buffer;
    renderer->material_count += materials.size();

    // update the descriptor for the materials
    VkDescriptorBufferInfo descriptor_buffer_info = vk_lib::descriptor_buffer_info(renderer->material_buffer.buffer);
    VkWriteDescriptorSet   descriptor_write =
        vk_lib::write_descriptor_set(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, renderer->asset_descriptor_set, nullptr, &descriptor_buffer_info);
    vkUpdateDescriptorSets(renderer->vk_context.device, 1, &descriptor_write, 0, nullptr);
}

static void renderer_add_textures(Renderer* renderer, std::span<Texture> textures) {

    std::vector<VkDescriptorImageInfo> descriptor_image_infos;
    descriptor_image_infos.reserve(textures.size());
    for (const Texture& texture : textures) {
        VkSampler             sampler    = texture.sampler == nullptr ? renderer->default_sampler : texture.sampler;
        VkDescriptorImageInfo image_info = vk_lib::descriptor_image_info(texture.image.image_view, texture.image.layout, sampler);
        descriptor_image_infos.push_back(image_info);
    }

    VkWriteDescriptorSet write_descriptor_set =
        vk_lib::write_descriptor_set(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, renderer->asset_descriptor_set, descriptor_image_infos.data(),
                                     nullptr, nullptr, renderer->texture_count, descriptor_image_infos.size());
    vkUpdateDescriptorSets(renderer->vk_context.device, 1, &write_descriptor_set, 0, nullptr);
    renderer->texture_count += textures.size();
}

static void renderer_init_shader_data(Renderer* renderer) {
    const VkContext* vk_ctx = &renderer->vk_context;

    constexpr uint32_t variable_texture_count = 5000;

    VkDescriptorPoolSize       scene_data_pool_size = vk_lib::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3);
    VkDescriptorPoolSize       materials_pool_size  = vk_lib::descriptor_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1);
    VkDescriptorPoolSize       textures_pool_size   = vk_lib::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, variable_texture_count);
    std::array                 pool_sizes           = {scene_data_pool_size, materials_pool_size, textures_pool_size};
    VkDescriptorPoolCreateInfo descriptor_pool_ci   = vk_lib::descriptor_pool_create_info(4, pool_sizes);

    VK_CHECK(vkCreateDescriptorPool(vk_ctx->device, &descriptor_pool_ci, nullptr, &renderer->descriptor_pool));

    // scene descriptor layout
    VkDescriptorSetLayoutBinding    scene_data_layout_binding      = vk_lib::descriptor_set_layout_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    std::array                      scene_layout_bindings          = {scene_data_layout_binding};
    VkDescriptorSetLayoutCreateInfo scene_descriptor_set_layout_ci = vk_lib::descriptor_set_layout_create_info(scene_layout_bindings);
    vkCreateDescriptorSetLayout(vk_ctx->device, &scene_descriptor_set_layout_ci, nullptr, &renderer->scene_descriptor_set_layout);

    // assets descriptor layout
    VkDescriptorSetLayoutBinding materials_layout_binding = vk_lib::descriptor_set_layout_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    VkDescriptorSetLayoutBinding textures_layout_binding =
        vk_lib::descriptor_set_layout_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, variable_texture_count);
    std::array                                  asset_layout_bindings = {materials_layout_binding, textures_layout_binding};
    std::array<VkDescriptorBindingFlags, 2>     binding_flags         = {0, VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT};
    VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_ci{};
    binding_flags_ci.sType                                   = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    binding_flags_ci.bindingCount                            = binding_flags.size();
    binding_flags_ci.pBindingFlags                           = binding_flags.data();
    binding_flags_ci.pNext                                   = nullptr;
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_ci = vk_lib::descriptor_set_layout_create_info(asset_layout_bindings, 0, &binding_flags_ci);
    vkCreateDescriptorSetLayout(vk_ctx->device, &descriptor_set_layout_ci, nullptr, &renderer->asset_descriptor_set_layout);

    // scene descriptors allocation
    std::array scene_set_layouts = {renderer->scene_descriptor_set_layout, renderer->scene_descriptor_set_layout,
                                    renderer->scene_descriptor_set_layout};
    renderer->scene_descriptor_sets.resize(renderer->frames.size());
    VkDescriptorSetAllocateInfo scene_desc_set_ai =
        vk_lib::descriptor_set_allocate_info(scene_set_layouts.data(), renderer->descriptor_pool, renderer->frames.size());
    VK_CHECK(vkAllocateDescriptorSets(vk_ctx->device, &scene_desc_set_ai, renderer->scene_descriptor_sets.data()));

    // asset descriptor allocation
    VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variable_info{};
    variable_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
    variable_info.descriptorSetCount = 1;
    variable_info.pDescriptorCounts  = &variable_texture_count;
    variable_info.descriptorSetCount = 1;
    VkDescriptorSetAllocateInfo asset_desc_set_ai =
        vk_lib::descriptor_set_allocate_info(&renderer->asset_descriptor_set_layout, renderer->descriptor_pool, 1, &variable_info);
    VK_CHECK(vkAllocateDescriptorSets(vk_ctx->device, &asset_desc_set_ai, &renderer->asset_descriptor_set));

    // create default material
    TextureInfo default_texture_info{};
    default_texture_info.index     = 0;
    default_texture_info.tex_coord = 0;

    Material default_material{};
    default_material.base_color_texture         = default_texture_info;
    default_material.normal_texture             = default_texture_info;
    default_material.metallic_roughness_texture = default_texture_info;
    default_material.base_color_factors         = {1, 1, 1, 1};
    default_material.metallic_factor            = 1;
    default_material.roughness_factor           = 1;

    std::array default_materials = {default_material};
    renderer_add_materials(renderer, default_materials);

    // create default sampler
    VkSamplerCreateInfo default_sampler_ci = vk_lib::sampler_create_info();
    VK_CHECK(vkCreateSampler(renderer->vk_context.device, &default_sampler_ci, nullptr, &renderer->default_sampler));

    // create image with one pixel?
    VkBufferCreateInfo      staging_buf_ci = vk_lib::buffer_create_info(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 4);
    VmaAllocationCreateInfo staging_buf_allocation_ci{};
    staging_buf_allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    staging_buf_allocation_ci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    AllocatedBuffer staging_buffer;
    VK_CHECK(vmaCreateBuffer(renderer->allocator, &staging_buf_ci, &staging_buf_allocation_ci, &staging_buffer.buffer, &staging_buffer.allocation,
                             &staging_buffer.allocation_info));

    uint8_t image_data[4] = {255, 255, 255, 255};
    memcpy(staging_buffer.allocation_info.pMappedData, image_data, 4);

    VkImageCreateInfo default_tex_image_ci =
        vk_lib::image_create_info(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, vk_lib::extent_3d(1, 1));
    VmaAllocationCreateInfo default_tex_allocation_ci{};
    default_tex_allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    vmaCreateImage(renderer->allocator, &default_tex_image_ci, &default_tex_allocation_ci, &renderer->default_texture_image.image,
                   &renderer->default_texture_image.allocation, &renderer->default_texture_image.allocation_info);

    VkImageSubresourceRange subresource_range = vk_lib::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

    VkImageViewCreateInfo default_tex_image_view_ci =
        vk_lib::image_view_create_info(VK_FORMAT_R8G8B8A8_UNORM, renderer->default_texture_image.image, &subresource_range);
    VK_CHECK(vkCreateImageView(vk_ctx->device, &default_tex_image_view_ci, nullptr, &renderer->default_texture_image.image_view));

    vk_command_immediate_submit(
        renderer->vk_context.device, renderer->vk_context.frame_command_pool, renderer->vk_context.graphics_queue, [&](VkCommandBuffer cmd_buf) {
            VkImageMemoryBarrier2 pre_transfer_memory_barrier = vk_lib::image_memory_barrier_2(
                renderer->default_texture_image.image, subresource_range, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                renderer->vk_context.queue_family, renderer->vk_context.queue_family);

            VkDependencyInfo pre_transfer_dependency_info = vk_lib::dependency_info(&pre_transfer_memory_barrier, nullptr, nullptr);
            vkCmdPipelineBarrier2(cmd_buf, &pre_transfer_dependency_info);

            VkImageSubresourceLayers image_subresource_layers = vk_lib::image_subresource_layers(VK_IMAGE_ASPECT_COLOR_BIT);
            VkBufferImageCopy        copy_region              = vk_lib::buffer_image_copy(image_subresource_layers, vk_lib::extent_3d(1, 1));
            vkCmdCopyBufferToImage(cmd_buf, staging_buffer.buffer, renderer->default_texture_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                                   &copy_region);

            VkImageMemoryBarrier2 texture_use_memory_barrier = vk_lib::image_memory_barrier_2(
                renderer->default_texture_image.image, subresource_range, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, renderer->vk_context.queue_family, renderer->vk_context.queue_family);

            VkDependencyInfo texture_use_dependency_info = vk_lib::dependency_info(&texture_use_memory_barrier, nullptr, nullptr);
            vkCmdPipelineBarrier2(cmd_buf, &texture_use_dependency_info);
        });

    renderer->default_texture_image.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    Texture default_texture{};
    default_texture.image   = renderer->default_texture_image;
    default_texture.sampler = renderer->default_sampler;

    std::array default_textures = {default_texture};
    renderer_add_textures(renderer, default_textures);

    // scene data buffers
    VkBufferCreateInfo      scene_buf_ci = vk_lib::buffer_create_info(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(SceneData));
    VmaAllocationCreateInfo scene_buf_allocation_ci{};
    scene_buf_allocation_ci.usage = VMA_MEMORY_USAGE_AUTO;
    scene_buf_allocation_ci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    for (uint32_t i = 0; i < renderer->frames.size(); i++) {
        AllocatedBuffer scene_buffer;
        VK_CHECK(vmaCreateBuffer(renderer->allocator, &scene_buf_ci, &scene_buf_allocation_ci, &scene_buffer.buffer, &scene_buffer.allocation,
                                 &scene_buffer.allocation_info));

        renderer->scene_data_buffers.push_back(scene_buffer);
    }

    vmaDestroyBuffer(renderer->allocator, staging_buffer.buffer, staging_buffer.allocation);
}

void renderer_add_gltf_asset(Renderer* renderer, const char* gltf_path) {
    LoadOptions gltf_load_options{};
    gltf_load_options.gltf_path = gltf_path;
    gltf_load_options.cache_dir = "cache/";

    GltfAsset asset = load_gltf(&gltf_load_options, renderer->vk_context.device, renderer->allocator, renderer->vk_context.frame_command_pool,
                                renderer->vk_context.graphics_queue, renderer->vk_context.queue_family);

    // add new draw objects
    for (const GltfNode& node : asset.nodes) {
        if (!node.mesh.has_value()) {
            // only renderer nodes with meshes
            continue;
        }
        const GltfMesh* gltf_mesh = &asset.meshes[node.mesh.value()];
        for (const GltfPrimitive& gltf_primitive : gltf_mesh->primitives) {
            DrawObject new_draw_object{};
            new_draw_object.transform = glm::make_mat4(node.world_transform);

            if (gltf_primitive.index_buffer.has_value()) {
                new_draw_object.index_buffer = gltf_primitive.index_buffer.value();
            } else {
                abort_message("currently not handling GLTF assets without index buffers");
            }

            new_draw_object.index_count   = gltf_primitive.index_count;
            new_draw_object.index_type    = gltf_primitive.index_type;
            new_draw_object.vertex_buffer = gltf_primitive.vertex_buffer;

            if (gltf_primitive.material.has_value()) {
                // offset the material index by how many materials we already have from other gltf assets
                new_draw_object.material_index = gltf_primitive.material.value() + renderer->material_count;

                if (asset.materials[gltf_primitive.material.value()].alpha_mode == GltfAlphaMode::opaque) {
                    renderer->opaque_draws.push_back(new_draw_object);
                } else {
                    renderer->transparent_draws.push_back(new_draw_object);
                }
            } else {
                // default material index
                new_draw_object.material_index = 0;
                // assume opaque when no material
                renderer->opaque_draws.push_back(new_draw_object);
            }
        }
    }

    // add new materials
    std::vector<Material> materials;
    materials.reserve(asset.materials.size());
    for (const GltfMaterial& gltf_material : asset.materials) {
        Material new_material{};
        new_material.base_color_factors = glm::vec4(gltf_material.base_color_factors[0], gltf_material.base_color_factors[1],
                                                    gltf_material.base_color_factors[2], gltf_material.base_color_factors[3]);
        new_material.metallic_factor    = gltf_material.metallic_factor;
        new_material.roughness_factor   = gltf_material.roughness_factor;

        // I will offset the texture indices by how many textures we already have from other gltf assets
        if (gltf_material.normal_texture.has_value()) {
            new_material.normal_texture = gltf_material.normal_texture.value();
            new_material.normal_texture.index += renderer->texture_count;
        }
        if (gltf_material.base_color_texture.has_value()) {
            new_material.base_color_texture = gltf_material.base_color_texture.value();
            new_material.base_color_texture.index += renderer->texture_count;
        }
        if (gltf_material.metallic_roughness_texture.has_value()) {
            new_material.metallic_roughness_texture = gltf_material.metallic_roughness_texture.value();
            new_material.metallic_roughness_texture.index += renderer->texture_count;
        }

        materials.push_back(new_material);
    }

    renderer_add_materials(renderer, materials);

    // add new textures
    std::vector<Texture> textures;
    textures.reserve(asset.textures.size());
    for (const GltfTexture& gltf_texture : asset.textures) {
        Texture new_texture{};

        // it would be really weird for a gltf_texture to not have an image btw. handle it anyway
        if (gltf_texture.image.has_value()) {
            new_texture.image = asset.images[gltf_texture.image.value()];
        } else {
            new_texture.image = renderer->default_texture_image;
        }

        if (gltf_texture.sampler.has_value()) {
            new_texture.sampler = asset.samplers[gltf_texture.sampler.value()];
        } else {
            new_texture.sampler = renderer->default_sampler;
        }

        textures.push_back(new_texture);
    }

    renderer_add_textures(renderer, textures);

    renderer->assets.push_back(asset);
}

static void renderer_update_scene_data(Renderer* renderer, uint32_t frame_index) {
    camera_update(renderer->frame_time);
    SceneData scene_data{};
    float aspect_ratio = static_cast<float>(renderer->swapchain_context.extent.width) / static_cast<float>(renderer->swapchain_context.extent.height);

    scene_data.proj = glm::perspective(glm::radians(70.f), aspect_ratio, 10000.f, 0.01f);
    scene_data.proj[1][1] *= -1;

    scene_data.view = camera_view();

    VK_CHECK(vmaCopyMemoryToAllocation(renderer->allocator, &scene_data, renderer->scene_data_buffers[frame_index].allocation, 0, sizeof(SceneData)));

    VkDescriptorBufferInfo buffer_info = vk_lib::descriptor_buffer_info(renderer->scene_data_buffers[frame_index].buffer);

    VkWriteDescriptorSet descriptor_write =
        vk_lib::write_descriptor_set(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, renderer->scene_descriptor_sets[frame_index], nullptr, &buffer_info);
    vkUpdateDescriptorSets(renderer->vk_context.device, 1, &descriptor_write, 0, nullptr);
}

void renderer_draw(Renderer* renderer) {
    static auto last_frame_time    = std::chrono::high_resolution_clock::now();
    auto        current_frame_time = std::chrono::high_resolution_clock::now();
    auto        duration           = std::chrono::duration<float>(current_frame_time - last_frame_time);
    float       raw_frame_time     = duration.count();

    constexpr float max_frame_time     = 0.1f; // 100ms (10 FPS)
    float           clamped_frame_time = std::min(raw_frame_time, max_frame_time);

    constexpr float smoothing_factor    = 0.5f;
    static float    smoothed_frame_time = 0.016f;
    smoothed_frame_time                 = smoothing_factor * clamped_frame_time + (1.0f - smoothing_factor) * smoothed_frame_time;

    renderer->frame_time = smoothed_frame_time;
    last_frame_time      = current_frame_time;

    VkContext*        vk_ctx        = &renderer->vk_context;
    SwapchainContext* swapchain_ctx = &renderer->swapchain_context;
    const uint32_t    frame_index   = renderer->curr_frame % swapchain_ctx->images.size();
    const Frame*      current_frame = &renderer->frames[frame_index];

    VkCommandBuffer command_buffer = current_frame->command_buffer;

    VK_CHECK(vkWaitForFences(vk_ctx->device, 1, &current_frame->in_flight_fence, true, UINT64_MAX));
    VK_CHECK(vkResetFences(vk_ctx->device, 1, &current_frame->in_flight_fence));

    renderer_update_scene_data(renderer, frame_index);

    uint32_t swapchain_image_index;
    VkResult swapchain_result = vkAcquireNextImageKHR(vk_ctx->device, swapchain_ctx->swapchain, UINT64_MAX, current_frame->image_available_semaphore,
                                                      nullptr, &swapchain_image_index);

    if (swapchain_result == VK_ERROR_OUT_OF_DATE_KHR || swapchain_result == VK_SUBOPTIMAL_KHR) {
        swapchain_context_recreate(swapchain_ctx, vk_ctx->physical_device, vk_ctx->device, vk_ctx->surface, renderer->window.glfw_window);
        destroy_render_resources(renderer);
        create_render_resources(renderer);
        return;
    }

    VK_CHECK(vkResetCommandBuffer(command_buffer, 0));

    VkCommandBufferBeginInfo begin_info = vk_lib::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(command_buffer, &begin_info));

    const VkViewport viewport = vk_lib::viewport(static_cast<float>(swapchain_ctx->extent.width), static_cast<float>(swapchain_ctx->extent.height));
    const VkRect2D   scissor  = vk_lib::rect_2d(swapchain_ctx->extent);

    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    const VkImageSubresourceRange color_subresource_range = vk_lib::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

    const VkImageMemoryBarrier2 msaa_draw_image_memory_barrier =
        vk_lib::image_memory_barrier_2(renderer->msaa_color_image.image, color_subresource_range, VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, vk_ctx->queue_family, vk_ctx->queue_family);

    const VkImageSubresourceRange depth_subresource_range = vk_lib::image_subresource_range(VK_IMAGE_ASPECT_DEPTH_BIT);

    const VkImageMemoryBarrier2 depth_image_memory_barrier =
        vk_lib::image_memory_barrier_2(renderer->depth_image.image, depth_subresource_range, VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, vk_ctx->queue_family, vk_ctx->queue_family);

    VkImage     swapchain_image      = swapchain_ctx->images[swapchain_image_index];
    VkImageView swapchain_image_view = swapchain_ctx->image_views[swapchain_image_index];

    const VkImageMemoryBarrier2 resolve_draw_image_memory_barrier =
        vk_lib::image_memory_barrier_2(swapchain_image, color_subresource_range, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                       vk_ctx->queue_family, vk_ctx->queue_family);

    std::array draw_image_memory_barriers = {msaa_draw_image_memory_barrier, resolve_draw_image_memory_barrier, depth_image_memory_barrier};

    const VkDependencyInfo draw_dependency_info = vk_lib::dependency_info_batch(draw_image_memory_barriers, {}, {});
    vkCmdPipelineBarrier2(command_buffer, &draw_dependency_info);

    VkClearValue clear_value{};
    clear_value.color = {0, 0, 0, 0};

    VkRenderingAttachmentInfo color_attachment_info = vk_lib::rendering_attachment_info(
        renderer->msaa_color_image.image_view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_DONT_CARE, &clear_value, VK_RESOLVE_MODE_AVERAGE_BIT, swapchain_image_view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    std::array color_attachment_infos = {color_attachment_info};

    VkRenderingAttachmentInfo depth_attachment_info =
        vk_lib::rendering_attachment_info(renderer->depth_image.image_view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                          VK_ATTACHMENT_STORE_OP_DONT_CARE, &clear_value);

    const VkRect2D           render_area    = vk_lib::rect_2d(swapchain_ctx->extent);
    const VkRenderingInfoKHR rendering_info = vk_lib::rendering_info(render_area, color_attachment_infos, &depth_attachment_info);

    vkCmdBeginRenderingKHR(command_buffer, &rendering_info);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->opaque_graphics_pipeline.pipeline);

    // both opaque and transparent have the same pipeline layouts. just use the opaque pipeline layout
    std::array desc_sets = {renderer->scene_descriptor_sets[frame_index], renderer->asset_descriptor_set};
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->opaque_graphics_pipeline.pipeline_layout, 0, desc_sets.size(),
                            desc_sets.data(), 0, nullptr);

    for (const DrawObject& opaque_draw : renderer->opaque_draws) {
        PushConstants push_constants{};
        push_constants.model_transform    = opaque_draw.transform;
        push_constants.vertex_buf_address = opaque_draw.vertex_buffer.address;
        push_constants.material_index     = opaque_draw.material_index;

        vkCmdPushConstants(command_buffer, renderer->opaque_graphics_pipeline.pipeline_layout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants),
                           &push_constants);

        vkCmdBindIndexBuffer(command_buffer, opaque_draw.index_buffer.buffer, 0, opaque_draw.index_type);
        vkCmdDrawIndexed(command_buffer, opaque_draw.index_count, 1, 0, 0, 0);
    }

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->transparent_graphics_pipeline.pipeline);

    for (const DrawObject& transparent_draw : renderer->transparent_draws) {
        PushConstants push_constants{};
        push_constants.model_transform    = transparent_draw.transform;
        push_constants.vertex_buf_address = transparent_draw.vertex_buffer.address;
        push_constants.material_index     = transparent_draw.material_index;

        vkCmdPushConstants(command_buffer, renderer->transparent_graphics_pipeline.pipeline_layout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants),
                           &push_constants);

        vkCmdBindIndexBuffer(command_buffer, transparent_draw.index_buffer.buffer, 0, transparent_draw.index_type);
        vkCmdDrawIndexed(command_buffer, transparent_draw.index_count, 1, 0, 0, 0);
    }

    vkCmdEndRenderingKHR(command_buffer);

    const VkImageMemoryBarrier2 resolve_present_image_memory_barrier = vk_lib::image_memory_barrier_2(
        swapchain_image, color_subresource_range, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, vk_ctx->queue_family,
        vk_ctx->queue_family, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_NONE);

    const VkDependencyInfo present_dependency_info = vk_lib::dependency_info(&resolve_present_image_memory_barrier, nullptr, nullptr);
    vkCmdPipelineBarrier2(command_buffer, &present_dependency_info);

    VK_CHECK(vkEndCommandBuffer(command_buffer));

    VkCommandBufferSubmitInfo command_buffer_submit_info = vk_lib::command_buffer_submit_info(current_frame->command_buffer);
    VkSemaphoreSubmitInfo     wait_semaphore_submit_info =
        vk_lib::semaphore_submit_info(current_frame->image_available_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR);
    VkSemaphoreSubmitInfo signal_semaphore_submit_info =
        vk_lib::semaphore_submit_info(current_frame->render_finished_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR);

    VkSubmitInfo2 submit_info_2 = vk_lib::submit_info_2(&command_buffer_submit_info, &wait_semaphore_submit_info, &signal_semaphore_submit_info);

    VK_CHECK(vkQueueSubmit2(vk_ctx->graphics_queue, 1, &submit_info_2, current_frame->in_flight_fence));

    VkPresentInfoKHR present = vk_lib::present_info(&swapchain_ctx->swapchain, &swapchain_image_index, &current_frame->render_finished_semaphore);

    swapchain_result = vkQueuePresentKHR(vk_ctx->present_queue, &present);

    if (swapchain_result == VK_ERROR_OUT_OF_DATE_KHR || swapchain_result == VK_SUBOPTIMAL_KHR) {
        swapchain_context_recreate(swapchain_ctx, vk_ctx->physical_device, vk_ctx->device, vk_ctx->surface, renderer->window.glfw_window);
        destroy_render_resources(renderer);
        create_render_resources(renderer);
        return;
    }
    renderer->curr_frame++;
}

Renderer renderer_create() {

    Renderer renderer{};

    renderer.window     = window_create();
    renderer.vk_context = vk_context_create(renderer.window.glfw_window);
    VkContext* vk_ctx   = &renderer.vk_context;

    renderer.swapchain_context = swapchain_context_create(vk_ctx->physical_device, vk_ctx->device, vk_ctx->surface, renderer.window.glfw_window);
    const SwapchainContext* swapchain_ctx = &renderer.swapchain_context;

    const VkCommandPoolCreateInfo command_pool_ci =
        vk_lib::command_pool_create_info(vk_ctx->queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    vkCreateCommandPool(vk_ctx->device, &command_pool_ci, nullptr, &vk_ctx->frame_command_pool);

    renderer.allocator = allocator_create(&renderer.vk_context);

    create_render_resources(&renderer);

    renderer.frames = frames_create(vk_ctx->device, vk_ctx->frame_command_pool, swapchain_ctx->image_views, swapchain_ctx->images,
                                    vk_ctx->queue_family, renderer.msaa_color_image.image_view);

    renderer_init_shader_data(&renderer);

    renderer_create_graphics_pipeline(&renderer, vk_ctx->device, swapchain_ctx->surface_format.format);

    // renderer_add_gltf_asset(&renderer, "../assets/main1_sponza/NewSponza_Main_glTF_003.gltf");
    renderer_add_gltf_asset(&renderer, "../assets/DamagedHelmet.glb");

    return renderer;
}