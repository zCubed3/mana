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

#include <mana/internal/vulkan_window.hpp>

using namespace ManaVK;
using namespace ManaVK::Internal;

#include <iostream>

#define LOG_INLINE(args) std::cout << "[ManaVK::Internal::VulkanInstance]: "<< args
#define LOG(args) LOG_INLINE(args) << std::endl

void VulkanInstance::create_vk_instance(const InstanceSettings& settings) {
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
        auto result = filter_extensions(settings.extensions);
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
}

VulkanInstance::FilterResults<VulkanInstance::VulkanExtension> VulkanInstance::filter_extensions(const std::vector<VulkanExtension> &list) {
    // Query all the instance extensions first
    std::vector<VkExtensionProperties> supported_extensions;
    uint32_t extension_count = 0;

    // TODO: Query with layer names too?
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    supported_extensions.resize(extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, supported_extensions.data());

    return filter<VulkanExtension>(
            list,
            [supported_extensions]
            (VulkanInstance *p_instance, const VulkanExtension& extension) -> bool {
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
// Setup stages
//
void VulkanInstance::init_spawn_window(const VulkanInstance::WindowSettings& settings) {
    main_window = new VulkanWindow(settings.title, settings.width, settings.height);
}

void VulkanInstance::init_create_instance(const VulkanInstance::InstanceSettings& settings) {
    create_vk_instance(settings);
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

std::vector<VulkanInstance::VulkanExtension> VulkanInstance::get_sdl_extensions() {
    if (main_window == nullptr) {
        throw std::runtime_error("Main window is nullptr! Have you called init_spawn_window() yet?");
    }

    std::vector<const char*> extensions;
    uint32_t extension_count = 0;

    SDL_Vulkan_GetInstanceExtensions(nullptr, &extension_count, nullptr);
    extensions.resize(extension_count);
    SDL_Vulkan_GetInstanceExtensions(nullptr, &extension_count, extensions.data());

    std::vector<VulkanExtension> extension_list;

    extension_list.reserve(extensions.size());
    for (const auto& extension : extensions) {
        extension_list.emplace_back(extension, true);
    }

    return extension_list;
}