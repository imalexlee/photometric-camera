#include "loader.h"

#include <stb_image.h>

#define KHRONOS_STATIC


#include <ktx.h>
#include <ktxvulkan.h>
#include <thread>

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

void get_format_for_image(const cgltf_data* cgltf_data, uint32_t image_index, uint32_t color_channels, VkFormat* uncompressed_vk_format,
                          VkFormat* ideal_compressed_vk_format, ktx_transcode_fmt_e* ktx_transcode_format) {
    const cgltf_image* target_image = &cgltf_data->images[image_index];
    bool is_srgb                    = true; // default to sRGB, we'll set to false for data textures

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
        *uncompressed_vk_format = VK_FORMAT_R8_UNORM;
        *ideal_compressed_vk_format = VK_FORMAT_BC4_UNORM_BLOCK;
        *ktx_transcode_format       = KTX_TTF_BC4_R;
        break;
    case 2:
        *uncompressed_vk_format = VK_FORMAT_R8G8_UNORM;
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

// load gltf images, compress them, then create vulkan images and images views from them
void load_gltf_images(const LoadOptions* load_options, const cgltf_data* cgltf_data, VkPhysicalDevice physical_device, VkDevice device, VkQueue queue,
                      VkCommandPool command_pool, std::vector<GltfImages>* images) {
    // if cache dir doesn't exist, create it.
    // for each image hash, if the cache contains that ktx file, use that
    // if it doesn't contain the hash, write the ktx file to the cache
    // take the ktx file, transcode it to appropriate compression
    // upload to vulkan and create a VkImage and VkImageView from it
    bool check_cache    = false;
    bool write_to_cache = false;
    if (!load_options->cache_dir.empty()) {
        if (std::filesystem::exists(load_options->cache_dir)) {
            // cache already exists, so check for ktx images there for the duration of this function
            check_cache    = true;
            write_to_cache = true;
        } else {
            // check_cache will remain false since this new cache will not have any cached images within it obviously
            write_to_cache = std::filesystem::create_directory(load_options->cache_dir);
        }
    }

    // initialize vulkan stuff for libktx
    // ktxVulkanDeviceInfo ktx_vk_device_info;
    // ktxVulkanDeviceInfo_Construct(&ktx_vk_device_info, physical_device, device, queue, command_pool, nullptr);

    images->clear();
    images->reserve(cgltf_data->images_count);
    for (uint32_t i = 0; i < cgltf_data->images_count; i++) {
        //1 . pull format from images
        int width, height, component_count;
        const cgltf_buffer_view* buffer_view = cgltf_data->images[i].buffer_view;
        stbi_info_from_memory(static_cast<uint8_t*>(buffer_view->buffer->data) + buffer_view->offset,
                              static_cast<int>(buffer_view->size), &width, &height, &component_count);

        ktx_transcode_fmt_e ktx_transcode_format{};
        VkFormat uncompressed_format{};
        VkFormat compressed_format{}; // may not need this
        get_format_for_image(cgltf_data, i, component_count, &uncompressed_format, &compressed_format, &ktx_transcode_format);

        // 2. get a ktx file for this image
        // create variable for ktx texture and for cache_img_exists = false;
        // if check cache enabled, look for a ktx file there and set ktx texture to that if it exists.
        // if no ktx texture, then create a ktx file by loading with stb, then using lib ktx to write the ktx file
        // if use_cache_dir == true and cache_img_exists == false, write the newly created ktx to the cache for next time

        ktxTexture2* ktx_texture = nullptr;
        KTX_error_code result;

        std::string image_hash = load_options->gltf_path.stem().string() + "_" + std::to_string(i) + "_" + ".ktx2";
        std::string cache_path =
            load_options->cache_dir.string() + image_hash;

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
                result = ktxTexture2_WriteToNamedFile(ktx_texture, cache_path.c_str());
                if (result != KTX_SUCCESS) {
                    const std::string message = "Cannot write ktx data to file with error code: " + result;
                    abort_message(message);
                }
            }
        }
        // now I have a filled ktx texture. transcode it appropriately
        result = ktxTexture2_TranscodeBasis(ktx_texture, ktx_transcode_format, 0);
        if (result != KTX_SUCCESS) {
            const std::string message = "Cannot transcode with error code: " + result;
            abort_message(message);
        }

        // todo: create vulkan images and image_views

        ktxTexture2_Destroy(ktx_texture);
    }
}

void load_gltf(const LoadOptions* load_options, VkPhysicalDevice physical_device, VkDevice device, VkQueue queue, VkCommandPool command_pool) {
    cgltf_options options{};
    cgltf_data* gltf_data = nullptr;

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

    std::vector<GltfImages> gltf_images;
    load_gltf_images(load_options, gltf_data, physical_device, device, queue, command_pool, &gltf_images);

    cgltf_free(gltf_data);
}