#include "loader.h"

#include <stb_image.h>

#define KHRONOS_STATIC

#include <functional>
#include <ktx.h>
#include <renderer.h>
#include <thread>

void vk_command_immediate_submit(VkDevice device, VkCommandPool command_pool, VkQueue queue,
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

static std::string cgltf_result_to_string(cgltf_result result) {
    switch (result) {
    case cgltf_result_success:
        return "Success";
    case cgltf_result_data_too_short:
        return "Data too short";
    case cgltf_result_unknown_format:
        return "Unknown format";
    case cgltf_result_invalid_json:
        return "Invalid JSON";
    case cgltf_result_invalid_gltf:
        return "Invalid glTF";
    case cgltf_result_invalid_options:
        return "Invalid options";
    case cgltf_result_file_not_found:
        return "File not found";
    case cgltf_result_io_error:
        return "I/O error";
    case cgltf_result_out_of_memory:
        return "Out of memory";
    case cgltf_result_legacy_gltf:
        return "Legacy glTF";
    case cgltf_result_max_enum:
        return "Invalid enum value";
    default:
        return "Unknown result code";
    }
}

static void get_format_for_image(const cgltf_data* cgltf_data, uint32_t image_index, uint32_t color_channels, VkFormat* uncompressed_vk_format,
                                 VkFormat* ideal_compressed_vk_format, ktx_transcode_fmt_e* ktx_transcode_format) {
    const cgltf_image* target_image = &cgltf_data->images[image_index];
    bool               is_srgb      = true; // default to sRGB, we'll set to false for data textures

    for (uint32_t i = 0; i < cgltf_data->materials_count; i++) {
        const cgltf_material* material = &cgltf_data->materials[i];
        if (material->normal_texture.texture && material->normal_texture.texture->image == target_image) {
            is_srgb = false;
            break;
        }

        if (material->occlusion_texture.texture && material->occlusion_texture.texture->image == target_image) {
            is_srgb = false;
            break;
        }

        if (material->has_pbr_metallic_roughness && material->pbr_metallic_roughness.metallic_roughness_texture.texture &&
            material->pbr_metallic_roughness.metallic_roughness_texture.texture->image == target_image) {
            is_srgb = false;
            break;
        }

        if (material->has_pbr_specular_glossiness && material->pbr_specular_glossiness.specular_glossiness_texture.texture &&
            material->pbr_specular_glossiness.specular_glossiness_texture.texture->image == target_image) {
            is_srgb = false;
            break;
        }

        if (material->has_clearcoat) {
            if (material->clearcoat.clearcoat_roughness_texture.texture &&
                material->clearcoat.clearcoat_roughness_texture.texture->image == target_image) {
                is_srgb = false;
                break;
            }

            if (material->clearcoat.clearcoat_normal_texture.texture && material->clearcoat.clearcoat_normal_texture.texture->image == target_image) {
                is_srgb = false;
                break;
            }
        }

        if (material->has_transmission && material->transmission.transmission_texture.texture &&
            material->transmission.transmission_texture.texture->image == target_image) {
            is_srgb = false;
            break;
        }

        if (material->has_sheen && material->sheen.sheen_roughness_texture.texture &&
            material->sheen.sheen_roughness_texture.texture->image == target_image) {
            is_srgb = false;
            break;
        }
    }

    switch (color_channels) {
    case 1:
        *uncompressed_vk_format     = VK_FORMAT_R8_UNORM;
        *ideal_compressed_vk_format = VK_FORMAT_BC4_UNORM_BLOCK;
        *ktx_transcode_format       = KTX_TTF_BC4_R;
        break;
    case 2:
        *uncompressed_vk_format     = VK_FORMAT_R8G8_UNORM;
        *ideal_compressed_vk_format = VK_FORMAT_BC5_UNORM_BLOCK;
        *ktx_transcode_format       = KTX_TTF_BC5_RG;
        break;
    case 3:
        if (is_srgb) {
            *uncompressed_vk_format     = VK_FORMAT_R8G8B8_SRGB;
            *ideal_compressed_vk_format = VK_FORMAT_BC7_SRGB_BLOCK;
        } else {
            *uncompressed_vk_format     = VK_FORMAT_R8G8B8_UNORM;
            *ideal_compressed_vk_format = VK_FORMAT_BC7_UNORM_BLOCK;
        }
        *ktx_transcode_format = KTX_TTF_BC7_RGBA;
        break;
    case 4:
    default:
        if (is_srgb) {
            *uncompressed_vk_format     = VK_FORMAT_R8G8B8A8_SRGB;
            *ideal_compressed_vk_format = VK_FORMAT_BC7_SRGB_BLOCK;
        } else {
            *uncompressed_vk_format     = VK_FORMAT_R8G8B8A8_UNORM;
            *ideal_compressed_vk_format = VK_FORMAT_BC7_UNORM_BLOCK;
        }
        *ktx_transcode_format = KTX_TTF_BC7_RGBA;
        break;
    }
}

static void allocate_staging_buffer(VmaAllocator allocator, uint64_t data_size, AllocatedBuffer* staging_buffer) {
    const VkBufferCreateInfo staging_buffer_ci = vk_lib::buffer_create_info(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, data_size, 0);
    VmaAllocationCreateInfo  allocation_ci{};
    allocation_ci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    VK_CHECK(vmaCreateBuffer(allocator, &staging_buffer_ci, &allocation_ci, &staging_buffer->buffer, &staging_buffer->allocation,
                             &staging_buffer->allocation_info));
}

// load gltf images, compress them, then create vulkan images and images views from them
static std::vector<AllocatedImage> load_gltf_images(const LoadOptions* load_options, const cgltf_data* cgltf_data, VkDevice device,
                                                    VmaAllocator allocator, VkCommandPool command_pool, VkQueue queue, uint32_t queue_family_index,
                                                    AllocatedBuffer* staging_buffer) {
    bool check_cache    = false;
    bool write_to_cache = false;
    if (!load_options->cache_dir.empty()) {
        if (std::filesystem::exists(load_options->cache_dir)) {
            check_cache    = true;
            write_to_cache = true;
        } else {
            // will write to cache only if caller provided a cache directory and the image was not found in the cache
            write_to_cache = std::filesystem::create_directory(load_options->cache_dir);
        }
    }

    std::vector<AllocatedImage> gltf_images;
    gltf_images.reserve(cgltf_data->images_count);
    for (uint32_t i = 0; i < cgltf_data->images_count; i++) {

        // 1. pull formats from images
        int                      width, height, component_count;
        const cgltf_buffer_view* buffer_view = cgltf_data->images[i].buffer_view;
        stbi_info_from_memory(static_cast<uint8_t*>(buffer_view->buffer->data) + buffer_view->offset, static_cast<int>(buffer_view->size), &width,
                              &height, &component_count);

        ktx_transcode_fmt_e ktx_transcode_format{};
        VkFormat            uncompressed_format{};
        VkFormat            compressed_format{}; // may not need this
        get_format_for_image(cgltf_data, i, component_count, &uncompressed_format, &compressed_format, &ktx_transcode_format);

        ktxTexture2*   ktx_texture = nullptr;
        KTX_error_code result;

        // 2. look into cache for images, if the cache directory exists
        std::string image_hash = load_options->gltf_path.stem().string() + "_" + std::to_string(i) + ".ktx2";
        std::string cache_path = load_options->cache_dir.string() + image_hash;

        bool cache_img_exists = false;
        if (check_cache) {
            if (std::filesystem::exists(cache_path)) {
                cache_img_exists = true;
                result           = ktxTexture2_CreateFromNamedFile(cache_path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx_texture);
                if (result != KTX_SUCCESS) {
                    const std::string message = "Cannot load file with error code: " + result;
                    abort_message(message);
                }
            }
        }

        // 3. if caller didn't provide a cache directory, or if the image was not found there, load and compress the image now
        if (!cache_img_exists) {

            unsigned char* img_data = stbi_load_from_memory(static_cast<uint8_t*>(buffer_view->buffer->data) + buffer_view->offset,
                                                            static_cast<int>(buffer_view->size), &width, &height, &component_count, 0);
            if (img_data == nullptr) {
                abort_message(stbi_failure_reason());
            }

            ktxTextureCreateInfo create_info{};
            create_info.vkFormat        = uncompressed_format;
            create_info.baseDepth       = 1;
            create_info.baseWidth       = width;
            create_info.baseHeight      = height;
            create_info.numDimensions   = 2;
            create_info.numFaces        = 1;
            create_info.numLayers       = 1;
            create_info.numLevels       = 1;
            create_info.isArray         = false;
            create_info.generateMipmaps = false;
            result                      = ktxTexture2_Create(&create_info, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &ktx_texture);
            if (result != KTX_SUCCESS) {
                const std::string message = "Cannot create ktx texture with error code: " + result;
                abort_message(message);
            }

            uint64_t img_data_size = component_count * width * height;

            result = ktxTexture_SetImageFromMemory(ktxTexture(ktx_texture), 0, 0, 0, img_data, img_data_size);
            stbi_image_free(img_data); // todo: see if it is actually safe to free the image here

            if (result != KTX_SUCCESS) {
                const std::string message = "Cannot set ktx image from memory with error code: " + result;
                abort_message(message);
            }

            ktxBasisParams params{};
            params.structSize  = sizeof(params);
            params.uastc       = KTX_TRUE;
            params.threadCount = std::max(std::thread::hardware_concurrency(), 1u);

            result = ktxTexture2_CompressBasisEx(ktx_texture, &params);
            if (result != KTX_SUCCESS) {
                const std::string message = "Cannot compress with error code: " + result;
                abort_message(message);
            }
            if (write_to_cache) {
                // 4. write the ktx texture to the cache directory only if the user provided a directory and if the image wasn't found in the cache
                result = ktxTexture2_WriteToNamedFile(ktx_texture, cache_path.c_str());
                if (result != KTX_SUCCESS) {
                    const std::string message = "Cannot write ktx data to file with error code: " + result;
                    abort_message(message);
                }
            }
        }

        // 5. now that I have a ktx texture, transcode it appropriately
        if (ktxTexture2_NeedsTranscoding(ktx_texture)) {
            result = ktxTexture2_TranscodeBasis(ktx_texture, ktx_transcode_format, 0);
            if (result != KTX_SUCCESS) {
                const std::string message = "Cannot transcode with error code: " + result;
                abort_message(message);
            }
        }

        VkExtent3D        base_image_extent = vk_lib::extent_3d(ktx_texture->baseWidth, ktx_texture->baseHeight);
        VkImageCreateInfo image_ci =
            vk_lib::image_create_info(static_cast<VkFormat>(ktx_texture->vkFormat), VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                      base_image_extent, VK_IMAGE_LAYOUT_UNDEFINED, ktx_texture->numLevels);

        VmaAllocationCreateInfo texture_allocation_ci{};
        texture_allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        AllocatedImage new_texture{};
        new_texture.extent = base_image_extent;
        vmaCreateImage(allocator, &image_ci, &texture_allocation_ci, &new_texture.image, &new_texture.allocation, &new_texture.allocation_info);

        // create image view
        VkImageSubresourceRange subresource_range = vk_lib::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT, ktx_texture->numLevels);
        VkImageViewCreateInfo   image_view_ci =
            vk_lib::image_view_create_info(static_cast<VkFormat>(ktx_texture->vkFormat), new_texture.image, &subresource_range);
        vkCreateImageView(device, &image_view_ci, nullptr, &new_texture.image_view);

        // 6. allocate additional memory in the staging buffer allocation if needed for this texture
        if (staging_buffer->allocation_info.size < ktx_texture->dataSize) {
            if (staging_buffer->allocation_info.size > 0) {
                vmaDestroyBuffer(allocator, staging_buffer->buffer, staging_buffer->allocation);
            }
            allocate_staging_buffer(allocator, ktx_texture->dataSize, staging_buffer);
        }
        // 7. fill staging buffer with all image levels
        memcpy(staging_buffer->allocation_info.pMappedData, ktx_texture->pData, ktx_texture->dataSize);

        // 8. create copy info for each mip level for our staging_buffer -> image copy
        std::vector<VkBufferImageCopy> buffer_image_copies;
        buffer_image_copies.reserve(ktx_texture->numLevels);
        for (uint32_t mip_level = 0; mip_level < ktx_texture->numLevels; mip_level++) {
            size_t ktx_offset;
            result = ktxTexture2_GetImageOffset(ktx_texture, mip_level, 0, 0, &ktx_offset);
            if (result != KTX_SUCCESS) {
                const std::string message = "Cannot get offset into ktx image error code: " + result;
                abort_message(message);
            }

            VkImageSubresourceLayers image_subresource = vk_lib::image_subresource_layers(VK_IMAGE_ASPECT_COLOR_BIT, mip_level);
            VkExtent3D               image_extent      = vk_lib::extent_3d(ktx_texture->baseWidth >> mip_level, ktx_texture->baseHeight >> mip_level);
            VkBufferImageCopy        buffer_image_copy = vk_lib::buffer_image_copy(image_subresource, image_extent, ktx_offset);

            buffer_image_copies.push_back(buffer_image_copy);
        }

        // 9. run copy commands to upload the staging data to image memory

        vk_command_immediate_submit(device, command_pool, queue, [&](VkCommandBuffer cmd_buf) {
            const VkImageMemoryBarrier2 copy_memory_barrier =
                vk_lib::image_memory_barrier_2(new_texture.image, subresource_range, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                               queue_family_index, queue_family_index);

            const VkDependencyInfo copy_dependency_info = vk_lib::dependency_info(&copy_memory_barrier, {}, {});
            vkCmdPipelineBarrier2(cmd_buf, &copy_dependency_info);

            vkCmdCopyBufferToImage(cmd_buf, staging_buffer->buffer, new_texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   buffer_image_copies.size(), buffer_image_copies.data());

            const VkImageMemoryBarrier2 texture_use_memory_barrier =
                vk_lib::image_memory_barrier_2(new_texture.image, subresource_range, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, queue_family_index, queue_family_index);

            const VkDependencyInfo texture_use_dependency_info = vk_lib::dependency_info(&texture_use_memory_barrier, {}, {});
            vkCmdPipelineBarrier2(cmd_buf, &texture_use_dependency_info);
        });

        ktxTexture2_Destroy(ktx_texture);

        gltf_images.push_back(new_texture);
    }

    return gltf_images;
}

static std::vector<VkSampler> load_gltf_samplers(const cgltf_data* cgltf_data, VkDevice device) {
    std::vector<VkSampler> samplers;
    samplers.reserve(cgltf_data->samplers_count);

    for (uint32_t i = 0; i < cgltf_data->samplers_count; i++) {
        const cgltf_sampler* cgltf_sampler = &cgltf_data->samplers[i];

        VkSamplerCreateInfo sampler_info = {};
        sampler_info.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        switch (cgltf_sampler->mag_filter) {
        case cgltf_filter_type_nearest:
        case cgltf_filter_type_nearest_mipmap_nearest:
        case cgltf_filter_type_nearest_mipmap_linear:
            sampler_info.magFilter = VK_FILTER_NEAREST;
            break;
        default:
            sampler_info.magFilter = VK_FILTER_LINEAR;
            break;
        }

        switch (cgltf_sampler->min_filter) {
        case cgltf_filter_type_nearest:
        case cgltf_filter_type_nearest_mipmap_nearest:
            sampler_info.minFilter  = VK_FILTER_NEAREST;
            sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;

        case cgltf_filter_type_linear:
        case cgltf_filter_type_linear_mipmap_nearest:
            sampler_info.minFilter  = VK_FILTER_LINEAR;
            sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;

        case cgltf_filter_type_nearest_mipmap_linear:
            sampler_info.minFilter  = VK_FILTER_NEAREST;
            sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            break;

        default:
            sampler_info.minFilter  = VK_FILTER_LINEAR;
            sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            break;
        }

        switch (cgltf_sampler->wrap_s) {
        case cgltf_wrap_mode_clamp_to_edge:
            sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            break;
        case cgltf_wrap_mode_mirrored_repeat:
            sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            break;
        default:
            sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            break;
        }

        switch (cgltf_sampler->wrap_t) {
        case cgltf_wrap_mode_clamp_to_edge:
            sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            break;

        case cgltf_wrap_mode_mirrored_repeat:
            sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            break;
        default:
            sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            break;
        }

        sampler_info.addressModeW            = sampler_info.addressModeV; // Usually the same as V
        sampler_info.anisotropyEnable        = VK_TRUE;
        sampler_info.maxAnisotropy           = 16.0f;
        sampler_info.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        sampler_info.unnormalizedCoordinates = VK_FALSE;
        sampler_info.compareEnable           = VK_FALSE;
        sampler_info.compareOp               = VK_COMPARE_OP_NEVER;
        sampler_info.mipLodBias              = 0.0f;
        sampler_info.minLod                  = 0.0f;
        sampler_info.maxLod                  = VK_LOD_CLAMP_NONE;

        VkSampler sampler;
        VK_CHECK(vkCreateSampler(device, &sampler_info, nullptr, &sampler));

        samplers.push_back(sampler);
    }

    return samplers;
}

// create meshes along with primitives. allocate vertex and index buffers on gpu
static std::vector<GltfMesh> load_gltf_meshes(const cgltf_data* cgltf_data, VkDevice device, VkQueue queue, VkCommandPool command_pool,
                                              VmaAllocator allocator, AllocatedBuffer* staging_buffer) {
    std::vector<GltfMesh> meshes;
    meshes.reserve(cgltf_data->meshes_count);

    for (uint32_t i = 0; i < cgltf_data->meshes_count; i++) {
        const cgltf_mesh*          gltf_mesh = &cgltf_data->meshes[i];
        std::vector<GltfPrimitive> primitives;
        primitives.reserve(gltf_mesh->primitives_count);
        for (uint32_t j = 0; j < gltf_mesh->primitives_count; j++) {
            const cgltf_primitive* gltf_primitive = &gltf_mesh->primitives[j];

            GltfPrimitive primitive{};
            primitive.material = -1; // default is no material
            if (gltf_primitive->material) {
                primitive.material = static_cast<int32_t>(gltf_primitive->material - cgltf_data->materials);
            }

            if (gltf_primitive->type != cgltf_primitive_type_invalid) {
                primitive.mode = static_cast<PrimitiveMode>(gltf_primitive->type - 1);
            }
            // load indices if present
            if (gltf_primitive->indices) {
                const cgltf_accessor* indices_accessor = gltf_primitive->indices;
                if (gltf_primitive->indices->component_type == cgltf_component_type_r_32u) {
                    primitive.index_type = VK_INDEX_TYPE_UINT32;
                }
                // up the size of the staging buffer if needed
                if (staging_buffer->allocation_info.size < indices_accessor->buffer_view->size) {
                    if (staging_buffer->allocation_info.size > 0) {
                        vmaDestroyBuffer(allocator, staging_buffer->buffer, staging_buffer->allocation);
                    }
                    allocate_staging_buffer(allocator, indices_accessor->buffer_view->size, staging_buffer);
                }
                memcpy(staging_buffer->allocation_info.pMappedData,
                       static_cast<uint8_t*>(indices_accessor->buffer_view->buffer->data) + indices_accessor->buffer_view->offset,
                       indices_accessor->buffer_view->size);

                VkBufferCopy buffer_copy = vk_lib::buffer_copy(indices_accessor->buffer_view->size);

                vk_command_immediate_submit(device, command_pool, queue, [&](VkCommandBuffer cmd_buf) {
                    vkCmdCopyBuffer(cmd_buf, staging_buffer->buffer, primitive.index_buffer, 1, &buffer_copy);
                });
            }

            // todo: load attributes
        }
    }

    return meshes;
}

void load_gltf(const LoadOptions* load_options, VkDevice device, VmaAllocator allocator, VkCommandPool command_pool, VkQueue queue,
               uint32_t queue_family_index) {
    cgltf_options options{};
    cgltf_data*   gltf_data = nullptr;

    cgltf_result result = cgltf_parse_file(&options, load_options->gltf_path.string().c_str(), &gltf_data);
    if (result != cgltf_result_success) {
        const std::string message = "Cannot parse gltf/glb file with error: " + cgltf_result_to_string(result);
        abort_message(message);
    }

    result = cgltf_load_buffers(&options, gltf_data, load_options->gltf_path.string().c_str());
    if (result != cgltf_result_success) {
        const std::string message = "Cannot parse gltf/glb file with error: " + cgltf_result_to_string(result);
        abort_message(message);
    }

    result = cgltf_validate(gltf_data);
    if (result != cgltf_result_success) {
        const std::string message = "Cannot parse gltf/glb file with error: " + cgltf_result_to_string(result);
        abort_message(message);
    }

    AllocatedBuffer staging_buffer{};

    std::vector<AllocatedImage> gltf_images =
        load_gltf_images(load_options, gltf_data, device, allocator, command_pool, queue, queue_family_index, &staging_buffer);
    // std::vector<VkSampler> gltf_samplers = load_gltf_samplers(gltf_data, device);
    // load an array of meshes
    // load_gltf_meshes(gltf_data, device, queue, command_pool, allocator, &staging_buffer);

    if (staging_buffer.allocation_info.size > 0) {
        vmaDestroyBuffer(allocator, staging_buffer.buffer, staging_buffer.allocation);
    }
    cgltf_free(gltf_data);
}