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

#include "vulkan_instance.hpp"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <SDL_vulkan.h>

#include <mana/internal/vulkan_queue.hpp>
#include <mana/internal/vulkan_window.hpp>

using namespace ManaVK;
using namespace ManaVK::Internal;

#include <iostream>

#define LOG_INLINE(args) std::cout << "[ManaVK::Internal::VulkanInstance]: "<< args
#define LOG(args) LOG_INLINE(args) << std::endl

//
// Setup stages
//
void VulkanInstance::init_spawn_window(const VulkanInstance::WindowSettings& settings) {
    main_window = new VulkanWindow(settings.title, settings.width, settings.height);
}

void VulkanInstance::init_create_instance(const VulkanInstance::InstanceSettings& settings) {
    //
    // Create the VkInstance
    //
    VkApplicationInfo app_info{};
    {
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

        app_info.apiVersion = settings.api_version;

        app_info.pEngineName = settings.engine_name.c_str();
        app_info.pApplicationName = settings.app_name.c_str();

        app_info.engineVersion = settings.engine_version;
        app_info.applicationVersion = settings.app_version;
    }

    std::vector<const char *> enabled_extensions;
    {
        // This time, any missing extensions are an error!
        auto result = filter_instance_extensions(settings.extensions);
        if (!result.missing.empty()) {
            for (const auto &missing: result.missing) {
                LOG("Error! Missing extension '" << missing.get_name() << "'...");
            }

            throw std::runtime_error("Unsupported instance extensions were found! Check stdout for more info");
        }

        for (const auto &extension: settings.extensions) {
            enabled_extensions.emplace_back(extension.get_cstr());
        }
    }

    std::vector<const char *> enabled_layers;
    {
        // This time, any missing layers are an error!
        auto result = filter_layers(settings.layers);
        if (!result.missing.empty()) {
            for (const auto &missing: result.missing) {
                LOG("Error! Missing layer '" << missing.get_name() << "'...");
            }

            throw std::runtime_error("Unsupported instance layers were found! Check stdout for more info");
        }

        for (const auto &layer: settings.layers) {
            enabled_layers.emplace_back(layer.get_cstr());
        }
    }

    VkInstanceCreateInfo create_info{};
    {
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        create_info.pApplicationInfo = &app_info;

        create_info.ppEnabledExtensionNames = enabled_extensions.data();
        create_info.enabledExtensionCount = enabled_extensions.size();

        create_info.ppEnabledLayerNames = enabled_layers.data();
        create_info.enabledLayerCount = enabled_layers.size();
    }

    VkResult result = vkCreateInstance(&create_info, nullptr, &vk_instance);

    if (result != VK_SUCCESS) {
        LOG("vkCreateInstance failed with error code (" << result << ")");
        throw std::runtime_error("vkCreateInstance failed! Check stdout for more info!");
    }

    //
    // Setup main_window further
    //
    main_window->create_surface(vk_instance);
}

void VulkanInstance::init_find_gpu(const GPUPreferences &prefs) {
    if (vk_instance == nullptr) {
        throw std::runtime_error("vk_instance is nullptr! Have you called init_create_instance()?");
    }

    if (main_window->get_vk_surface() == nullptr) {
        throw std::runtime_error("main_window surface is nullptr! Have you called init_create_instance()?");
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

        // TODO: Make VulkanInstance logging optional
        {
            LOG("========== GPU #" << gpu_number - 1 << " ==========");
            LOG("NAME: " << properties.deviceName);
            LOG("API VERSION: " << std::hex << "0x" << properties.apiVersion << std::dec);
            LOG("VENDOR: " << std::hex << "0x" << properties.vendorID << std::dec);
            LOG("============================");
        }

        uint32_t queue_index = 0;

        std::vector<VulkanQueue::Type> requested_queues;
        {
            if (prefs.need_graphics_queue) {
                requested_queues.push_back(VulkanQueue::Type::Graphics);
            }

            if (prefs.need_transfer_queue) {
                requested_queues.push_back(VulkanQueue::Type::Transfer);
            }

            if (prefs.need_present_queue) {
                requested_queues.push_back(VulkanQueue::Type::Present);
            }
        }

        std::vector<VulkanQueue*> found_queues;

        // TODO: Better queue exclusivity?
        for (VkQueueFamilyProperties queue_family: queue_families) {
            for (auto& type : requested_queues) {
                // Ensure we haven't already found a suitable queue
                bool already_found = false;

                for (auto& queue : found_queues) {
                    if (queue->get_type() == type) {
                        already_found = true;
                    }
                }

                if (already_found) {
                    continue;
                }

                bool unique = false;
                VulkanQueue *queue = new VulkanQueue(type, queue_index);

                switch (type) {
                    case VulkanQueue::Type::Graphics: {
                        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                            found_queues.push_back(queue);
                            unique = true;
                        }

                        break;
                    }

                    case VulkanQueue::Type::Transfer: {
                        if (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                            found_queues.push_back(queue);
                            unique = true;
                        }

                        break;
                    }

                        // TODO: Does every window need a unique present queue
                    case VulkanQueue::Type::Present: {
                        VkBool32 surface_support = false;
                        vkGetPhysicalDeviceSurfaceSupportKHR(gpu, queue_index, main_window->get_vk_surface(), &surface_support);

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

        if (found_queues.size() != requested_queues.size()) {
            LOG("Warning: GPU #" << gpu_number - 1 << "is missing required queues!");
            continue;
        }

        // TODO: This will break if Vulkan ever stops using VkBool32 for features
        bool has_features = true;
        {
            size_t num_features = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);

            // TODO: Hope this doesn't shit the bed and break everything one day
            const VkBool32 *has_ptr = &features.robustBufferAccess;
            const VkBool32 *wants_ptr = &prefs.required_features.robustBufferAccess;

            for (int f = 0; f < num_features; f++) {
                VkBool32 has = (has_ptr)[f];
                VkBool32 requested = (wants_ptr)[f];

                if (requested && !has) {
                    has_features = false;
                }
            }
        }

        if (!has_features) {
            throw std::runtime_error("Requested features were not supported!");
        }

        // TODO: Allow the user to request if a feature is supported?

        vk_gpu = gpu;

        for (auto queue : found_queues) {
            if (queue->get_type() == VulkanQueue::Type::Graphics) {
                queue_graphics = queue;
            }

            if (queue->get_type() == VulkanQueue::Type::Present) {
                queue_present = queue;
            }

            if (queue->get_type() == VulkanQueue::Type::Transfer) {
                queue_transfer = queue;
            }
        }

        return;
    }

    throw std::runtime_error("Failed to find a supported GPU!");
}

void VulkanInstance::init_create_device(const VulkanInstance::DeviceSettings &settings) {
    //
    // Create the device
    //
    if (vk_instance == nullptr) {
        throw std::runtime_error("vk_instance is nullptr! Have you called init_create_instance()?");
    }

    if (vk_gpu == nullptr) {
        throw std::runtime_error("vk_gpu is nullptr! Have you called init_find_gpu()?");
    }

    std::vector<VulkanQueue*> gpu_queues
    {
        queue_present,
        queue_graphics,
        queue_transfer
    };

    float queue_priority = 1.0F;
    std::vector<VkDeviceQueueCreateInfo> device_queue_infos;

    for (const auto queue: gpu_queues) {
        if (queue == nullptr) {
            continue;
        }

        VkDeviceQueueCreateInfo queue_info{};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = queue->get_index();
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &queue_priority;

        device_queue_infos.push_back(queue_info);
    }

    std::vector<const char *> enabled_extensions;
    {
        // This time, any missing extensions are an error!
        auto result = filter_device_extensions(settings.extensions);
        if (!result.missing.empty()) {
            for (const auto &missing: result.missing) {
                LOG("Error! Missing extension '" << missing.get_name() << "'...");
            }

            throw std::runtime_error("Unsupported device extensions were found! Check stdout for more info");
        }

        for (const auto &extension: settings.extensions) {
            enabled_extensions.emplace_back(extension.get_cstr());
        }
    }

    VkDeviceCreateInfo device_create_info{};
    {
        device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        device_create_info.pQueueCreateInfos = device_queue_infos.data();
        device_create_info.queueCreateInfoCount = static_cast<uint32_t>(device_queue_infos.size());

        device_create_info.ppEnabledExtensionNames = enabled_extensions.data();
        device_create_info.enabledExtensionCount = static_cast<uint32_t>(enabled_extensions.size());

        // TODO: Device layers?
        device_create_info.enabledLayerCount = 0;
    }


    VkResult result = vkCreateDevice(vk_gpu, &device_create_info, nullptr, &vk_device);

    if (result != VK_SUCCESS) {
        LOG("vkCreateDevice failed with error code (" << result << ")");
        throw std::runtime_error("vkCreateDevice failed! Please check the log above for more info!");
    }

    //
    // Setup the queue references
    //
    for (auto& queue : gpu_queues) {
        queue->warm_queue(vk_instance, vk_device);
    }
}

//
// Filtering
//
VulkanInstance::FilterResults<VulkanInstance::VulkanLayer> VulkanInstance::filter_layers(const std::vector<VulkanLayer> &list) {
    // Query all the instance layers first
    std::vector<VkLayerProperties> supported_layers;
    uint32_t layer_count = 0;

    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    supported_layers.resize(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, supported_layers.data());

    // TODO: Filter by version?
    return filter<VulkanLayer>(
        list,
        [supported_layers]
        (VulkanInstance *p_instance, const VulkanLayer& layer) -> bool {
            for (const auto& supported : supported_layers) {
                if (strcmp(supported.layerName, layer.get_name().c_str()) == 0) {
                    return true;
                }
            }

            return false;
        }
    );
}

VulkanInstance::FilterResults<VulkanInstance::VulkanInstanceExtension>
    VulkanInstance::filter_instance_extensions(const std::vector<VulkanInstanceExtension> &list)
{
    // Query all the instance extensions first
    std::vector<VkExtensionProperties> supported_extensions;
    uint32_t extension_count = 0;

    // TODO: Query with layer names too?
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    supported_extensions.resize(extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, supported_extensions.data());

    return filter<VulkanInstanceExtension>(
            list,
            [supported_extensions]
            (VulkanInstance *p_instance, const VulkanInstanceExtension& extension) -> bool {
                for (const auto& supported : supported_extensions) {
                    if (strcmp(supported.extensionName, extension.get_name().c_str()) == 0) {
                        return true;
                    }
                }

                return false;
            }
    );
}

VulkanInstance::FilterResults<VulkanInstance::VulkanDeviceExtension>
    VulkanInstance::filter_device_extensions(const std::vector<VulkanDeviceExtension> &list)
{
    if (vk_gpu == nullptr) {
        throw std::runtime_error("vk_gpu was nullptr! Have you called init_find_gpu() prior to filtering device extensions?");
    }

    // Query all the instance extensions first
    std::vector<VkExtensionProperties> supported_extensions;
    uint32_t extension_count = 0;

    // TODO: Query with layer names too?
    vkEnumerateDeviceExtensionProperties(vk_gpu, nullptr, &extension_count, nullptr);
    supported_extensions.resize(extension_count);
    vkEnumerateDeviceExtensionProperties(vk_gpu, nullptr, &extension_count, supported_extensions.data());

    return filter<VulkanDeviceExtension>(
            list,
            [supported_extensions]
            (VulkanInstance *p_instance, const VulkanDeviceExtension& extension) -> bool {
                for (const auto& supported : supported_extensions) {
                    if (strcmp(supported.extensionName, extension.get_name().c_str()) == 0) {
                        return true;
                    }
                }

                return false;
            }
    );
}

//
// Helpers
//
std::vector<VulkanInstance::VulkanInstanceExtension> VulkanInstance::get_sdl_extensions() {
    if (main_window == nullptr) {
        throw std::runtime_error("Main window is nullptr! Have you called init_spawn_window() yet?");
    }

    std::vector<const char*> extensions;
    uint32_t extension_count = 0;

    SDL_Vulkan_GetInstanceExtensions(nullptr, &extension_count, nullptr);
    extensions.resize(extension_count);
    SDL_Vulkan_GetInstanceExtensions(nullptr, &extension_count, extensions.data());

    std::vector<VulkanInstanceExtension> extension_list;

    extension_list.reserve(extensions.size());
    for (const auto& extension : extensions) {
        extension_list.emplace_back(extension, true);
    }

    return extension_list;
}