/*
MIT License

Copyright (c) 2023 zCubed (Liam R.)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "vulkan_provider.hpp"

#include <iostream>
#include <stdexcept>
#include <vector>

#include <SDL.h>
#include <SDL_vulkan.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <engine.hpp>
#include <window.hpp>

#include <data/size_tools.hpp>

#include <graphics/memory_block.hpp>
#include <graphics/render_pass.hpp>
#include <graphics/shader.hpp>
#include <graphics/targets/window_render_target.hpp>

#include <shader_gen/fallback.spv.vert.gen.h>
#include <shader_gen/fallback.spv.frag.gen.h>

using namespace Sapphire;

//
// VulkanProvider
//
bool Graphics::VulkanProvider::validate_instance_extensions(const std::vector<const char*> &extensions, Sapphire::Engine *p_engine) {
    std::vector<const char*> missing_extensions {};

    std::vector<VkExtensionProperties> supported_extensions;
    uint32_t extension_count;

    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    supported_extensions.resize(extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, supported_extensions.data());

    if (p_engine->has_verbosity(Engine::VerbosityFlags::Graphics)) {
        LOG_GRAPHICS("Listing supported instance extensions...");

        for (auto supported : supported_extensions) {
            LOG_GRAPHICS("\t" << supported.extensionName);
        }
    }

    for (auto requested : extensions) {
        bool has_support = false;

        if (p_engine->has_verbosity(Engine::VerbosityFlags::Graphics)) {
            LOG_GRAPHICS("Checking if extension '" << requested << "' is supported...");
        }

        for (auto supported : supported_extensions) {
            if (strcmp(requested, supported.extensionName) == 0) {
                has_support = true;
                break;
            }
        }

        if (!has_support) {
            LOG_GRAPHICS("Error: Extension '" << requested << "' is not supported by this device!");
            missing_extensions.emplace_back(requested);
        }
    }

    return missing_extensions.empty();
}

bool Graphics::VulkanProvider::validate_instance_layers(const std::vector<const char*> &layers, Sapphire::Engine *p_engine) {
    std::vector<const char*> missing_layers {};

    std::vector<VkLayerProperties> supported_layers;
    uint32_t layer_count;

    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    supported_layers.resize(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, supported_layers.data());

    if (p_engine->has_verbosity(Engine::VerbosityFlags::Graphics)) {
        LOG_GRAPHICS("Listing supported instance layers...");

        for (auto supported : supported_layers) {
            LOG_GRAPHICS("\t" << supported.layerName);
        }
    }

    for (auto requested : layers) {
        bool has_support = false;

        if (p_engine->has_verbosity(Engine::VerbosityFlags::Graphics)) {
            LOG_GRAPHICS("Checking if layer '" << requested << "' is supported...");
        }

        for (auto supported : supported_layers) {
            if (strcmp(requested, supported.layerName) == 0) {
                has_support = true;
                break;
            }
        }

        if (!has_support) {
            LOG_GRAPHICS("Warning: Layer '" << requested << "' is not supported by this device, this is a non-fatal error!");
            missing_layers.emplace_back(requested);
        }
    }

    return missing_layers.empty();
}

bool Graphics::VulkanProvider::validate_device_extensions(const std::vector<const char*> &extensions, Sapphire::Engine *p_engine) {
    std::vector<const char*> missing_extensions {};

    std::vector<VkExtensionProperties> supported_extensions;
    uint32_t extension_count;

    vkEnumerateDeviceExtensionProperties(vk_gpu, nullptr, &extension_count, nullptr);
    supported_extensions.resize(extension_count);
    vkEnumerateDeviceExtensionProperties(vk_gpu, nullptr, &extension_count, supported_extensions.data());

    if (p_engine->has_verbosity(Engine::VerbosityFlags::Graphics)) {
        LOG_GRAPHICS("Listing supported instance extensions...");

        for (auto supported : supported_extensions) {
            LOG_GRAPHICS("\t" << supported.extensionName);
        }
    }

    for (auto requested : extensions) {
        bool has_support = false;

        if (p_engine->has_verbosity(Engine::VerbosityFlags::Graphics)) {
            LOG_GRAPHICS("Checking if extension '" << requested << "' is supported...");
        }

        for (auto supported : supported_extensions) {
            if (strcmp(requested, supported.extensionName) == 0) {
                has_support = true;
                break;
            }
        }

        if (!has_support) {
            LOG_GRAPHICS("Error: Extension '" << requested << "' is not supported by this GPU or system!");
            missing_extensions.emplace_back(requested);
        }
    }

    return missing_extensions.empty();
}

void Graphics::VulkanProvider::cache_surface_info(VkSurfaceKHR vk_surface) {
    uint32_t enumeration_count;

    vkGetPhysicalDeviceSurfaceFormatsKHR(vk_gpu, vk_surface, &enumeration_count, nullptr);
    vk_supported_surface_formats.resize(enumeration_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(vk_gpu, vk_surface, &enumeration_count, vk_supported_surface_formats.data());

    if (enumeration_count == 0) {
        throw std::runtime_error("No surface formats found! Did your drivers crash?");
    }

    enumeration_count = 0;

    vkGetPhysicalDeviceSurfacePresentModesKHR(vk_gpu, vk_surface, &enumeration_count, nullptr);
    vk_supported_present_modes.resize(enumeration_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(vk_gpu, vk_surface, &enumeration_count, vk_supported_present_modes.data());

    if (enumeration_count == 0) {
        throw std::runtime_error("No present modes found! Did your drivers crash?");
    }
}

VkFormat Graphics::VulkanProvider::find_supported_surface_format(const std::vector<VkFormat>& vk_formats) {
    for (VkFormat vk_format: vk_formats) {
        for (VkSurfaceFormatKHR vk_surface_format: vk_supported_surface_formats) {
            if (vk_surface_format.format == vk_format && vk_surface_format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
                return vk_format;
            }
        }
    }

    throw std::runtime_error("No supported surface format found!");
}

VkFormat Graphics::VulkanProvider::find_supported_format(const std::vector<VkFormat> &vk_formats, VkImageTiling vk_tiling, VkFormatFeatureFlags vk_feature_flags) {
    for (VkFormat vk_attempt_format: vk_formats) {
        VkFormatProperties vk_format_properties;
        vkGetPhysicalDeviceFormatProperties(vk_gpu, vk_attempt_format, &vk_format_properties);

        if (vk_tiling == VK_IMAGE_TILING_LINEAR && (vk_format_properties.linearTilingFeatures & vk_feature_flags) == vk_feature_flags) {
            return vk_attempt_format;
        }

        if (vk_tiling == VK_IMAGE_TILING_OPTIMAL && (vk_format_properties.optimalTilingFeatures & vk_feature_flags) == vk_feature_flags) {
            return vk_attempt_format;
        }
    }

    throw std::runtime_error("No supported format found!");
}

VkPresentModeKHR Graphics::VulkanProvider::find_supported_present_mode(const std::vector<VkPresentModeKHR> &vk_present_modes) {
    for (VkPresentModeKHR vk_present_mode: vk_present_modes) {
        for (VkPresentModeKHR vk_supported_present_mode: vk_supported_present_modes) {
            if (vk_present_mode == vk_supported_present_mode) {
                return vk_present_mode;
            }
        }
    }

    throw std::runtime_error("No supported present mode found!");
}

void Graphics::VulkanProvider::determine_present_info() {
    // TODO: Make these configurable
    const bool sRGB = false;
    const bool d32 = false;
    const bool vsync = false;

    // Try to guess a fitting present format
    std::vector<VkFormat> vk_formats;
    std::vector<VkFormat> vk_depth_formats;

    if (sRGB) {
        vk_formats.push_back(VK_FORMAT_B8G8R8A8_SRGB);
    } else {
        vk_formats.push_back(VK_FORMAT_B8G8R8A8_UNORM);
    }

    if (d32) {
        vk_formats.push_back(VK_FORMAT_D32_SFLOAT_S8_UINT);
    }

    // We always push the D24 and D16 formats regardless
    vk_depth_formats.push_back(VK_FORMAT_D24_UNORM_S8_UINT);
    vk_depth_formats.push_back(VK_FORMAT_D16_UNORM_S8_UINT);

    // Try to guess a fitting present mode
    std::vector<VkPresentModeKHR> vk_present_modes;

    if (vsync) {
        vk_present_modes.push_back(VK_PRESENT_MODE_FIFO_KHR);
        vk_present_modes.push_back(VK_PRESENT_MODE_FIFO_RELAXED_KHR);
    }

    // We always want mailbox or immediate mode even if we request vsync
    vk_present_modes.push_back(VK_PRESENT_MODE_MAILBOX_KHR);
    vk_present_modes.push_back(VK_PRESENT_MODE_IMMEDIATE_KHR);

    // Then try to find them
    VkFormat vk_color_format = find_supported_surface_format(vk_formats);
    VkFormat vk_depth_format = find_supported_format(vk_depth_formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    VkPresentModeKHR vk_present_mode = find_supported_present_mode(vk_present_modes);

    present_info.vk_color_format = vk_color_format;
    present_info.vk_depth_format = vk_depth_format;
    present_info.vk_colorspace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    present_info.vk_present_mode = vk_present_mode;
}

void Graphics::VulkanProvider::create_instance(Sapphire::Engine *p_engine) {
    // TODO: Allow creation without a window
    VkInstanceCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    VkApplicationInfo app_info {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

    // TODO: Up the API version for newer features?
    // e.g. Raytracing? Video decoding?
    app_info.apiVersion = VK_API_VERSION_1_0;

    app_info.pEngineName = "Sapphire Engine";
    app_info.pApplicationName = p_engine->app_info.name.c_str();

    app_info.engineVersion = VK_MAKE_API_VERSION(
        0,
        p_engine->major_version,
        p_engine->minor_version,
        p_engine->patch_version
    );

    app_info.applicationVersion = VK_MAKE_API_VERSION(
        0,
        p_engine->app_info.major_version,
        p_engine->app_info.minor_version,
        p_engine->app_info.patch_version
    );

    create_info.pApplicationInfo = &app_info;

    std::vector<const char*> enabled_extensions;
    std::vector<const char*> enabled_layers;

    uint32_t extension_count;

    SDL_Vulkan_GetInstanceExtensions(p_engine->main_window->get_handle(), &extension_count, nullptr);
    enabled_extensions.resize(extension_count);
    SDL_Vulkan_GetInstanceExtensions(p_engine->main_window->get_handle(), &extension_count, enabled_extensions.data());

    // TODO: Make this toggleable with config entries
    enabled_layers.emplace_back("VK_LAYER_KHRONOS_validation");

    // TODO: Add our own extensions

    if (!validate_instance_extensions(enabled_extensions, p_engine)) {
        throw std::runtime_error("Failed to validate instance extensions. Please check the log above for more info!");
    }

    bool has_layers = validate_instance_layers(enabled_layers, p_engine);

    create_info.ppEnabledExtensionNames = enabled_extensions.data();
    create_info.enabledExtensionCount = enabled_extensions.size();

    create_info.ppEnabledLayerNames = enabled_layers.data();
    create_info.enabledLayerCount = enabled_layers.size();

    VkResult result = vkCreateInstance(&create_info, nullptr, &vk_instance);

    if (result != VK_SUCCESS) {
        LOG_GRAPHICS("vkCreateInstance failed with error code (" << result << ")");
        throw std::runtime_error("vkCreateInstance failed! Please check the log above for more info!");
    }
}

void Graphics::VulkanProvider::find_gpu(Engine *p_engine, VkSurfaceKHR vk_surface) {
    if (vk_surface == nullptr) {
        throw std::runtime_error("vk_surface is nullptr!");
    }

    uint32_t gpu_count;
    std::vector<VkPhysicalDevice> gpus;

    vkEnumeratePhysicalDevices(vk_instance, &gpu_count, nullptr);
    gpus.resize(gpu_count);
    vkEnumeratePhysicalDevices(vk_instance, &gpu_count, gpus.data());

    uint32_t gpu_number = 0;
    for (VkPhysicalDevice gpu : gpus) {
        uint32_t enumeration_count;

        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
        std::vector<VkQueueFamilyProperties> queue_families;

        vkGetPhysicalDeviceProperties(gpu, &properties);
        vkGetPhysicalDeviceFeatures(gpu, &features);

        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &enumeration_count, nullptr);
        queue_families.resize(enumeration_count);
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &enumeration_count, queue_families.data());

        enumeration_count = 0;
        gpu_number += 1;

        if (p_engine->has_verbosity(Engine::VerbosityFlags::Graphics)) {
            LOG_GRAPHICS("GPU #" << gpu_number - 1 << ":");
            LOG_GRAPHICS("\tNAME: " << properties.deviceName);
            LOG_GRAPHICS("\tAPI VERSION: " << std::hex << "0x" << properties.apiVersion << std::dec);
            LOG_GRAPHICS("\tVENDOR: " << std::hex << "0x" << properties.vendorID << std::dec);
        }

        uint32_t queue_index = 0;
        std::vector<Queue> found_queues;

        std::vector<QueueType> needed_queues = {
            QueueType::Graphics,
            QueueType::Present,
            QueueType::Transfer
        };

        // TODO: Better queue exclusivity?
        for (VkQueueFamilyProperties queue_family: queue_families) {
            for (QueueType type : needed_queues) {
                // Ensure we haven't already found a suitable queue
                bool already_found = false;

                for (Queue& queue : found_queues) {
                    if (queue.type == type) {
                        already_found = true;
                    }
                }

                if (already_found) {
                    continue;
                }

                Queue queue {};
                queue.type = type;
                queue.family = queue_index;

                bool unique = false;

                switch (type) {
                    case QueueType::Graphics: {
                        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                            found_queues.push_back(queue);
                            unique = true;
                        }

                        break;
                    }

                    case QueueType::Transfer: {
                        if (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                            found_queues.push_back(queue);
                            unique = true;
                        }

                        break;
                    }

                    case QueueType::Present: {
                        VkBool32 surface_support = false;
                        vkGetPhysicalDeviceSurfaceSupportKHR(gpu, queue_index, vk_surface, &surface_support);

                        if (surface_support) {
                            found_queues.push_back(queue);
                            unique = true;
                        }

                        break;
                    }
                }

                if (unique) {
                    break;
                }
            }

            queue_index++;
        }

        // TODO: Optional required features
        // TODO: GPU ranking / picking
        // TODO: Config option to manually decide GPU?
        // TODO: Go through ALL GPUs and don't pick the first supported GPU as the default always!

        if (found_queues.size() != needed_queues.size()) {
            LOG_GRAPHICS("Warning: Unsupported GPU, #" << gpu_number - 1);
            continue;
        } else {
            if (p_engine->has_verbosity(Engine::VerbosityFlags::Graphics)) {
                LOG_GRAPHICS("GPU #" << gpu_number - 1 << " PASSED!");
            }
        }

        // TODO: This will break if Vulkan stops using VkBool32 for features
        VkPhysicalDeviceFeatures enabled_features {};
        size_t num_features = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);

        // Old cursed code from SUPER OLD Sapphire
        // TODO: Hope this doesn't shit the bed and break everything one day
        /*
        for (int f = 0; f < num_features; f++) {
            VkBool32 has = (&features.robustBufferAccess)[f];
            VkBool32 requested = (&p_create_info->vk_enabled_features.robustBufferAccess)[f];

            (&enabled_features.robustBufferAccess)[f] = has && requested;
        }
        */

        vk_gpu = gpu;
        vk_gpu_features = features;

        for (Queue queue : found_queues) {
            if (queue.type == QueueType::Graphics) {
                queue_graphics = queue;
            }

            if (queue.type == QueueType::Present) {
                queue_present = queue;
            }

            if (queue.type == QueueType::Transfer) {
                queue_transfer = queue;
            }
        }

        return;
    }

    throw std::runtime_error("Failed to find a supported GPU!");
}

void Graphics::VulkanProvider::create_device(Sapphire::Engine *p_engine) {
    if (p_engine == nullptr) {
        throw std::runtime_error("p_engine was nullptr!");
    }

    std::vector<uint32_t> device_queues;
    std::vector<VkDeviceQueueCreateInfo> device_queue_infos;

    float queue_priority = 1;

    std::vector<Queue*> gpu_queues {
        &queue_present,
        &queue_graphics,
        &queue_transfer
    };

    for (const Queue* queue : gpu_queues) {
        device_queues.push_back(queue->family);

        VkDeviceQueueCreateInfo queue_info {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = queue->family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &queue_priority;

        device_queue_infos.push_back(queue_info);
    }

    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    device_create_info.pQueueCreateInfos = device_queue_infos.data();
    device_create_info.queueCreateInfoCount = static_cast<uint32_t>(device_queue_infos.size());

    // TODO: Not enable every single feature on the GPU?
    device_create_info.pEnabledFeatures = &vk_gpu_features;

    // Extensions
    std::vector<const char*> enabled_extensions {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    // TODO: Optional extensions
    if (!validate_device_extensions(enabled_extensions, p_engine)) {
        throw std::runtime_error("This system doesn't support the required device extensions!");
    }

    device_create_info.ppEnabledExtensionNames = enabled_extensions.data();
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(enabled_extensions.size());

    device_create_info.enabledLayerCount = 0;

    VkResult result = vkCreateDevice(vk_gpu, &device_create_info, nullptr, &vk_device);

    if (result != VK_SUCCESS) {
        LOG_GRAPHICS("vkCreateDevice failed with error code (" << result << ")");
        throw std::runtime_error("vkCreateDevice failed! Please check the log above for more info!");
    }

    // Setup the queue references
    for (Queue* queue : gpu_queues) {
        vkGetDeviceQueue(vk_device, queue->family, 0, &queue->vk_queue);

        uint32_t flags;
        switch (queue->type) {
            default:
                flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                break;

            case QueueType::Transfer:
                flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
                break;
        }

        VkCommandPoolCreateInfo pool_create_info{};

        pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_create_info.flags = flags;
        pool_create_info.queueFamilyIndex = queue->family;

        result = vkCreateCommandPool(vk_device, &pool_create_info, nullptr, &queue->vk_pool);

        if (result != VK_SUCCESS) {
            LOG_GRAPHICS("Error: vkCreateCommandPool failed with error code (" << result << ")");
            throw std::runtime_error("vkCreateCommandPool failed! Please check the log above for more info!");
        }
    }
}

void Graphics::VulkanProvider::create_vma_allocator(Sapphire::Engine *p_engine) {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = vk_gpu;
    allocatorInfo.device = vk_device;
    allocatorInfo.instance = vk_instance;

    VkResult result = vmaCreateAllocator(&allocatorInfo, &vma_allocator);

    if (result != VK_SUCCESS) {
        LOG_GRAPHICS("Error: vkCreateCommandPool failed with error code (" << result << ")");
        throw std::runtime_error("vkCreateCommandPool failed! Please check the log above for more info!");
    }

    // We also pre-allocate our mesh, texture, and buffer block pools
    // They're usually clusters of a certain size
    // TODO: Allow the user to change the VRAM usage target?
    // TODO: Change generic to user?
    // TODO: Unify uniform and mesh blocks as per? https://developer.nvidia.com/vulkan-memory-management
    mp_mesh = new MemoryPool(
        this,
        SizeTools::mib_to_bytes(MP_MESH_STRIDE_MB),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        0
    );

    mp_texture = new MemoryPool(
        this,
        SizeTools::mib_to_bytes(MP_TEXTURE_STRIDE_MB),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        0
    );

    mp_buffer = new MemoryPool(
        this,
        SizeTools::mib_to_bytes(MP_BUFFER_STRIDE_MB),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        0
    );

    smp_staging = new StagingMemoryPool(
        this,
        SizeTools::mib_to_bytes(SMP_STAGING_STRIDE_MB),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
    );
}

// TODO: Will these ever need to be increased?
void Graphics::VulkanProvider::create_vk_descriptor_pool() {
    VkDescriptorPoolSize pool_sizes[] =
            {
                    {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                    {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                    {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                    {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
            };

    VkDescriptorPoolCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.poolSizeCount = 11;
    create_info.pPoolSizes = pool_sizes;
    create_info.maxSets = 1000;
    create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VkResult result = vkCreateDescriptorPool(vk_device, &create_info, nullptr, &vk_descriptor_pool);

    if (result != VK_SUCCESS) {
        LOG_GRAPHICS("Error: vkCreateDescriptorPool failed with error code (" << result << ")");
        throw std::runtime_error("vkCreateDescriptorPool failed! Please check the log above for more info!");
    }
}

void Graphics::VulkanProvider::create_render_passes() {
    RenderPassBuilder builder;

    ColorAttachmentInfo color_attachment;

    ColorAttachmentInfo color_info{};
    color_info.format = present_info.vk_color_format;
    color_info.final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    color_info.ref_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    /*
    DepthStencilAttachmentInfo depth_stencil_info{};
    depth_stencil_info.format = present_info.vk_depth_format;
    depth_stencil_info.final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_stencil_info.ref_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    */

    builder.push_color_attachment(color_info);
    //window_render_pass_builder.push_depth_attachment(&depth_stencil_info);

    vk_render_pass_window = builder.build(this);
}

void Graphics::VulkanProvider::create_vk_vtx_info() {
    vk_vtx_attributes.clear();

    uint32_t offset = 0;
    uint32_t stride = 0;

    // TODO: Make a builder for this?

    // Position data
    {
        VkVertexInputAttributeDescription description {};
        description.binding = 0;
        description.location = offset++;
        description.format = VK_FORMAT_R32G32B32_SFLOAT;
        description.offset = stride;

        stride += sizeof(float) * 3;
        vk_vtx_attributes.push_back(description);
    }

    // Normal data
    {
        VkVertexInputAttributeDescription description {};
        description.binding = 0;
        description.location = offset++;
        description.format = VK_FORMAT_R32G32B32_SFLOAT;
        description.offset = stride;

        stride += sizeof(float) * 3;
        vk_vtx_attributes.push_back(description);
    }

    // UV0 data
    {
        VkVertexInputAttributeDescription description {};
        description.binding = 0;
        description.location = offset++;
        description.format = VK_FORMAT_R32G32_SFLOAT;
        description.offset = stride;

        stride += sizeof(float) * 2;
        vk_vtx_attributes.push_back(description);
    }

    // Binding info
    vk_vtx_binding = {};
    vk_vtx_binding.binding = 0;
    vk_vtx_binding.stride = stride;
    vk_vtx_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // TODO: Instance rate?
}

void Graphics::VulkanProvider::warm_fallbacks() {
    //
    // Engine fallback shader
    //
    {
        std::vector<char> vertex_data(sizeof(FALLBACK_VERT_CONTENTS));
        memcpy(vertex_data.data(), FALLBACK_VERT_CONTENTS, sizeof(FALLBACK_VERT_CONTENTS));

        std::vector<char> fragment_data(sizeof(FALLBACK_FRAG_CONTENTS));
        memcpy(fragment_data.data(), FALLBACK_FRAG_CONTENTS, sizeof(FALLBACK_FRAG_CONTENTS));

        std::shared_ptr<ShaderModule> sm_vertex = std::make_shared<ShaderModule>(this, ShaderModule::ModuleType::Vertex, vertex_data);
        std::shared_ptr<ShaderModule> sm_fragment = std::make_shared<ShaderModule>(this, ShaderModule::ModuleType::Fragment, fragment_data);

        ShaderProperties props{};
        shader_fallback = std::make_shared<Shader>(this, props, sm_vertex, sm_fragment);
    }
}

VkSemaphore Graphics::VulkanProvider::create_vk_semaphore() {
    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkSemaphore vk_semaphore;
    VkResult result = vkCreateSemaphore(vk_device, &semaphore_create_info, nullptr, &vk_semaphore);

    if (result != VK_SUCCESS) {
        LOG_GRAPHICS("Error: vkCreateSemaphore failed with error code (" << result << ")");
        throw std::runtime_error("vkCreateSemaphore failed! Please check the log above for more info!");
    }

    return vk_semaphore;
}

VkFence Graphics::VulkanProvider::create_vk_fence() {
    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkFence vk_fence;

    VkResult result = vkCreateFence(vk_device, &fence_create_info, nullptr, &vk_fence);

    if (result != VK_SUCCESS) {
        LOG_GRAPHICS("Error: vkCreateFence failed with error code (" << result << ")");
        throw std::runtime_error("vkCreateFence failed! Please check the log above for more info!");
    }

    return vk_fence;
}

// TODO: Unify render target code
VkSurfaceKHR Graphics::VulkanProvider::create_vk_surface(Sapphire::Window *p_window) {
    if (p_window == nullptr) {
        throw std::runtime_error("p_window was nullptr!");
    }

    if (p_window->get_handle() == nullptr) {
        throw std::runtime_error("p_window handle was nullptr!");
    }

    VkSurfaceKHR vk_surface = nullptr;
    if (!SDL_Vulkan_CreateSurface(p_window->get_handle(), vk_instance, &vk_surface)) {
        throw std::runtime_error("SDL_Vulkan_CreateSurface failed!");
    }

    return vk_surface;
}

void Graphics::VulkanProvider::setup_window_render_target(WindowRenderTarget *p_target, Window *p_window) {
    if (p_window == nullptr) {
        throw std::runtime_error("p_window was nullptr!");
    }

    if (p_window->get_handle() == nullptr) {
        throw std::runtime_error("p_window handle was nullptr!");
    }

    if (p_target == nullptr) {
        throw std::runtime_error("p_target was nullptr!");
    }

    VkSurfaceKHR vk_surface = p_target->get_vk_surface();
    WindowRenderTargetData rt_data;

    // Refreshes the extent and view data
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_gpu, p_target->get_vk_surface(), &rt_data.vk_capabilities);

    // High dpi aware extent
    if (rt_data.vk_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        rt_data.vk_extent = rt_data.vk_capabilities.currentExtent;
    } else {
        int width, height;
        SDL_Vulkan_GetDrawableSize(p_window->get_handle(), &width, &height);

        VkExtent2D actual_extent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
        };

        actual_extent.width = std::min(
                std::max(actual_extent.width, rt_data.vk_capabilities.minImageExtent.width),
                rt_data.vk_capabilities.maxImageExtent.width);
        actual_extent.height = std::min(
                std::max(actual_extent.height, rt_data.vk_capabilities.minImageExtent.height),
                rt_data.vk_capabilities.maxImageExtent.height);

        rt_data.vk_extent = actual_extent;
    }

    // Swapchain creation
    uint32_t image_count = rt_data.vk_capabilities.minImageCount + 1;

    std::vector<uint32_t> device_queues {
        queue_graphics.family,
        queue_present.family
    };

    if (rt_data.vk_capabilities.maxImageCount > 0 && image_count > rt_data.vk_capabilities.maxImageCount) {
        image_count = rt_data.vk_capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchain_create_info{};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

    swapchain_create_info.surface = vk_surface;
    swapchain_create_info.minImageCount = image_count;
    swapchain_create_info.imageFormat = present_info.vk_color_format;
    swapchain_create_info.imageColorSpace = present_info.vk_colorspace;
    swapchain_create_info.imageExtent = rt_data.vk_extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    swapchain_create_info.preTransform = rt_data.vk_capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    swapchain_create_info.presentMode = present_info.vk_present_mode;
    swapchain_create_info.clipped = VK_TRUE;

    swapchain_create_info.oldSwapchain = rt_data.vk_swapchain;

    if (queue_graphics.family != queue_present.family) {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = static_cast<uint32_t>(device_queues.size());
        swapchain_create_info.pQueueFamilyIndices = device_queues.data();
    } else {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = nullptr;
    }

    VkResult result = vkCreateSwapchainKHR(vk_device, &swapchain_create_info, nullptr, &rt_data.vk_swapchain);

    if (result != VK_SUCCESS) {
        LOG_GRAPHICS("vkCreateSwapchainKHR failed with error code (" << result << ")");
        throw std::runtime_error("vkCreateSwapchainKHR failed! Please check the log above for more info!");
    }

    vkGetSwapchainImagesKHR(vk_device, rt_data.vk_swapchain, &image_count, nullptr);

    rt_data.vk_images.resize(image_count);
    rt_data.vk_image_views.resize(image_count);

    vkGetSwapchainImagesKHR(vk_device, rt_data.vk_swapchain, &image_count, rt_data.vk_images.data());

    for (uint32_t i = 0; i < image_count; i++) {
        VkImageViewCreateInfo view_create_info{};
        view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

        view_create_info.image = rt_data.vk_images[i];
        view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_create_info.format = present_info.vk_color_format;

        view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_create_info.subresourceRange.baseMipLevel = 0;
        view_create_info.subresourceRange.levelCount = 1;
        view_create_info.subresourceRange.baseArrayLayer = 0;
        view_create_info.subresourceRange.layerCount = 1;

        result = vkCreateImageView(vk_device, &view_create_info, nullptr, &rt_data.vk_image_views[i]);

        if (result != VK_SUCCESS) {
            LOG_GRAPHICS("vkCreateImageView failed with error code (" << result << ")");
            throw std::runtime_error("vkCreateImageView failed! Please check the log above for more info!");
        }
    }

    // Depth buffer
    /*
    ValImageCreateInfo depth_create_info {};
    depth_create_info.extent.width = vk_extent.width;
    depth_create_info.extent.height = vk_extent.height;
    depth_create_info.format = p_val_instance->present_info->vk_depth_format;
    depth_create_info.usage_flags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depth_create_info.aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;

    val_depth_image = ValImage::create(&depth_create_info, p_val_instance);
    */

    // Framebuffers
    rt_data.vk_framebuffers.resize(image_count);
    for (uint32_t i = 0; i < image_count; i++) {
        std::vector<VkImageView> attachments = {
                rt_data.vk_image_views[i],
                //val_depth_image->vk_image_view
        };

        VkFramebufferCreateInfo framebuffer_create_info{};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.renderPass = vk_render_pass_window;
        framebuffer_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebuffer_create_info.pAttachments = attachments.data();
        framebuffer_create_info.width = rt_data.vk_extent.width;
        framebuffer_create_info.height = rt_data.vk_extent.height;
        framebuffer_create_info.layers = 1;

        result = vkCreateFramebuffer(vk_device, &framebuffer_create_info, nullptr, &rt_data.vk_framebuffers[i]);

        if (result != VK_SUCCESS) {
            LOG_GRAPHICS("vkCreateFramebuffer failed with error code (" << result << ")");
            throw std::runtime_error("vkCreateFramebuffer failed! Please check the log above for more info!");
        }
    }

    // Clear the old data before setting it
    p_target->get_rt_data().release(this);
    p_target->set_rt_data(rt_data);
}

VkCommandBuffer Graphics::VulkanProvider::allocate_command_buffer(QueueType queue_type) {
    Queue queue = get_queue(queue_type);

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = queue.vk_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer vk_buffer = nullptr;
    VkResult result = vkAllocateCommandBuffers(vk_device, &alloc_info, &vk_buffer);

    if (result != VK_SUCCESS) {
        LOG_GRAPHICS("vkCreateImageView failed with error code (" << result << ")");
        throw std::runtime_error("vkCreateImageView failed! Please check the log above for more info!");
    }

    return vk_buffer;
}

void Graphics::VulkanProvider::free_command_buffer(QueueType queue_type, VkCommandBuffer vk_command_buffer) {
    Queue queue = get_queue(queue_type);

    vkFreeCommandBuffers(vk_device, queue.vk_pool, 1, &vk_command_buffer);
}

void Graphics::VulkanProvider::reset_render_fence() {
    VkResult result = vkResetFences(vk_device, 1, &vk_render_fence);

    if (result != VK_SUCCESS) {
        LOG_GRAPHICS("vkResetFences failed with error code (" << result << ")");
        throw std::runtime_error("vkResetFences failed! Please check the log above for more info!");
    }
}

void Graphics::VulkanProvider::initialize(Sapphire::Engine *p_engine) {
    if (p_engine == nullptr) {
        throw std::runtime_error("p_engine was nullptr!");
    }

    create_instance(p_engine);

    // We initialize an incomplete render target!
    // This is necessary for fetching the supported formats of the host OS
    // If said host OS is Windows, then this isn't optional! Thank you Microsoft :|
    VkSurfaceKHR vk_surface = nullptr;
    if (!SDL_Vulkan_CreateSurface(p_engine->main_window->get_handle(), vk_instance, &vk_surface)) {
        throw std::runtime_error("SDL_Vulkan_CreateSurface failed!");
    }

    // Then find a GPU for presentation
    find_gpu(p_engine, vk_surface);
    create_device(p_engine);

    // Then VMA
    create_vma_allocator(p_engine);

    // Then our necessary sync objects
    vk_image_available_semaphore = create_vk_semaphore();
    vk_render_finished_semaphore = create_vk_semaphore();
    vk_render_fence = create_vk_fence();

    // Then pools
    create_vk_descriptor_pool();

    // Then ultimately our swapchain / present formats
    cache_surface_info(vk_surface);
    determine_present_info();

    create_render_passes();
    create_vk_vtx_info();

    warm_fallbacks();

    // Finally, initialize the render target of the main window by hand
    p_engine->main_window->set_render_target(new Graphics::WindowRenderTarget(this, p_engine->main_window, vk_surface));
}

VkInstance Graphics::VulkanProvider::get_vk_instance() {
    return vk_instance;
}

VkDevice Graphics::VulkanProvider::get_vk_device() {
    return vk_device;
}

VmaAllocator Graphics::VulkanProvider::get_vma_allocator() {
    return vma_allocator;
}

VkSemaphore Graphics::VulkanProvider::get_image_available_semaphore() {
    return vk_image_available_semaphore;
}

VkSemaphore Graphics::VulkanProvider::get_render_finished_semaphore() {
    return vk_render_finished_semaphore;
}

VkFence Graphics::VulkanProvider::get_render_fence() {
    return vk_render_fence;
}

VkRenderPass Graphics::VulkanProvider::get_render_pass_window() {
    return vk_render_pass_window;
}

Graphics::VulkanProvider::Queue Graphics::VulkanProvider::get_queue(Graphics::VulkanProvider::QueueType type) {
    switch (type) {
        case QueueType::Transfer:
            return queue_transfer;

        case QueueType::Graphics:
            return queue_graphics;

        case QueueType::Present:
            return queue_present;

        default:
            throw std::runtime_error("Unknown queue type!");
    }
}

VkVertexInputBindingDescription Graphics::VulkanProvider::get_vk_vtx_binding() {
    return vk_vtx_binding;
}

std::vector<VkVertexInputAttributeDescription> Graphics::VulkanProvider::get_vk_vtx_attributes() {
    return vk_vtx_attributes;
}

std::shared_ptr<Graphics::Shader> Graphics::VulkanProvider::get_shader_fallback() {
    return shader_fallback;
}

void Graphics::VulkanProvider::flush() {
    smp_staging->flush(this);

    // Clear out any queued destructions
    for (const auto& release : deferred_releases) {
        release(this);
    }

    deferred_releases.clear();
}

void Graphics::VulkanProvider::begin_frame() {
    defer_release = true;
}

void Graphics::VulkanProvider::end_frame() {
    defer_release = false;
}

void Graphics::VulkanProvider::await_frame() {
    VulkanProvider::Queue queue = get_queue(VulkanProvider::QueueType::Graphics);

    if (vk_device != nullptr && vk_render_fence != nullptr) {
        vkWaitForFences(vk_device, 1, &vk_render_fence, VK_TRUE, UINT64_MAX);
        reset_render_fence();
    }
}

VkCommandBuffer Graphics::VulkanProvider::begin_upload(QueueType type) {
    VkCommandBuffer vk_upload_buffer = allocate_command_buffer(type);

    VkCommandBufferBeginInfo buffer_begin_info{};
    buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // TODO: Safety
    vkBeginCommandBuffer(vk_upload_buffer, &buffer_begin_info);

    return vk_upload_buffer;
}

// TODO: SAFETY!!!
// TODO: NOT IMMEDIATELY SUBMIT!!!
// TODO: SAFETY AROUND UPLOAD TEMP CMD BUFFERS
void Graphics::VulkanProvider::end_upload(QueueType type, VkCommandBuffer vk_cmd_buffer) {
    Queue queue = get_queue(type);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &vk_cmd_buffer;

    vkEndCommandBuffer(vk_cmd_buffer);

    // TODO: Multiple uploads at once?
    vkQueueSubmit(queue.vk_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue.vk_queue);

    free_command_buffer(type, vk_cmd_buffer);
}

bool Graphics::VulkanProvider::get_defer_release() const {
    return defer_release;
}

void Graphics::VulkanProvider::enqueue_release(const ReleaseFunction& function) {
    deferred_releases.emplace_back(function);
}

// Allocates the virtual block and provides owning VkBuffer
// Memory can be allocated but not uploaded prior to usage, please use MemoryBlock::is_uploaded() first!
std::shared_ptr<Graphics::MemoryBlock> Graphics::VulkanProvider::upload_memory(size_t size, void* src, AllocationType type) {
    MemoryPool *mp_dst = nullptr;

    switch (type) {
        case AllocationType::Mesh:
            mp_dst = mp_mesh;
            break;

        case AllocationType::Buffer:
            mp_dst = mp_buffer;
            break;

        case AllocationType::Texture:
            mp_dst = mp_texture;
            break;
    }

    auto dst = mp_dst->alloc(size);
    smp_staging->enqueue_upload(size, src, dst);

    return dst;
}
