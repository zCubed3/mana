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

#include "mana_instance.hpp"

#include <mana/internal/vulkan_instance.hpp>

#include <iostream>

using namespace ManaVK;

#define LOG_INLINE(args) std::cout << "[ManaVK::ManaInstance]: "<< args
#define LOG(args) LOG_INLINE(args) << std::endl

ManaInstance::ManaInstance(const ManaVK::ManaInstance::ManaConfig &config) {
    //
    // VulkanInstance bootstrapping
    //
    vulkan_instance = new Internal::VulkanInstance();

    // Window creation
    {
        Internal::VulkanInstance::WindowSettings window_settings;

        window_settings.title = config.window_settings.title;
        window_settings.width = config.window_settings.width;
        window_settings.height = config.window_settings.height;

        vulkan_instance->init_spawn_window(window_settings);
    }

    // Instance creation
    {
        Internal::VulkanInstance::InstanceSettings instance_settings;

        instance_settings.engine_name = config.engine_name;
        instance_settings.engine_version = VK_MAKE_API_VERSION(
            config.app_version.rev,
            config.app_version.major,
            config.app_version.minor,
            config.app_version.patch
        );

        instance_settings.app_name = config.app_name;
        instance_settings.app_version = VK_MAKE_API_VERSION(
            config.app_version.rev,
            config.app_version.major,
            config.app_version.minor,
            config.app_version.patch
        );

        instance_settings.api_version = get_api_version(config.features);

        // Setup and validate extensions
        auto requested_extensions = std::vector<Internal::VulkanInstance::VulkanInstanceExtension>();
        {
            auto sdl_extensions = vulkan_instance->get_sdl_extensions();
            requested_extensions.insert(requested_extensions.end(), sdl_extensions.begin(), sdl_extensions.end());
        }

        // TODO: Raytracing extensions

        {
            auto filtered = vulkan_instance->filter_instance_extensions(requested_extensions);

            std::string fatal_message = "Required:\n";
            std::string warn_message = "Optional:\n";

            bool missing_required = false;
            for (auto& extension : filtered.missing) {
                if (config.debugging.verbose) {
                    LOG("Missing extension '" << extension.get_name() << "'");
                    LOG("\tRequired? " << std::boolalpha << extension.is_required());
                }

                if (extension.is_required()) {
                    missing_required = true;

                    fatal_message += "\n";
                    fatal_message += extension.get_name();
                } else {
                    warn_message += "\n";
                    warn_message += extension.get_name();
                }
            }

            if (missing_required) {
                throw std::runtime_error("Missing vulkan instance extensions!\n\n" + fatal_message + "\n" + warn_message);
            }
        }

        // Setup and validate layers
        auto requested_layers = std::vector<Internal::VulkanInstance::VulkanLayer>();
        {
            if (config.debugging.enable_khronos_layer) {
                requested_layers.emplace_back("VK_LAYER_KHRONOS_validation", true);
            }
        }

        {
            auto filtered = vulkan_instance->filter_layers(requested_layers);

            std::string fatal_message = "Required:\n";
            std::string warn_message = "Optional:\n";

            bool missing_required = false;
            for (auto& layer : filtered.missing) {
                if (config.debugging.verbose) {
                    LOG("Missing layer '" << layer.get_name() << "'");
                    LOG("\tRequired? " << std::boolalpha << layer.is_required());
                }

                if (layer.is_required()) {
                    missing_required = true;

                    fatal_message += "\n";
                    fatal_message += layer.get_name();
                } else {
                    warn_message += "\n";
                    warn_message += layer.get_name();
                }
            }

            if (missing_required) {
                throw std::runtime_error("Missing vulkan instance layer!\n\n" + fatal_message + "\n" + warn_message);
            }
        }

        instance_settings.extensions = requested_extensions;
        instance_settings.layers = requested_layers;

        vulkan_instance->init_create_instance(instance_settings);
    }

    // GPU finding
    {
        Internal::VulkanInstance::GPUPreferences gpu_preferences;

        // TODO: Features, e.g. line rendering?

        vulkan_instance->init_find_gpu(gpu_preferences);
    }

    // Device creation
    {
        Internal::VulkanInstance::DeviceSettings device_settings;

        // Setup and filter extensions
        auto requested_extensions = std::vector<Internal::VulkanInstance::VulkanDeviceExtension>();
        {
            requested_extensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME, true);
        }

        {
            auto filtered = vulkan_instance->filter_device_extensions(requested_extensions);

            std::string fatal_message = "Required:\n";
            std::string warn_message = "Optional:\n";

            bool missing_required = false;
            for (auto& layer : filtered.missing) {
                if (config.debugging.verbose) {
                    LOG("Missing device extension '" << layer.get_name() << "'");
                    LOG("\tRequired? " << std::boolalpha << layer.is_required());
                }

                if (layer.is_required()) {
                    missing_required = true;

                    fatal_message += "\n";
                    fatal_message += layer.get_name();
                } else {
                    warn_message += "\n";
                    warn_message += layer.get_name();
                }
            }

            if (missing_required) {
                throw std::runtime_error("Missing device extensions!\n\n" + fatal_message + "\n" + warn_message);
            }
        }

        device_settings.extensions = requested_extensions;

        vulkan_instance->init_create_device(device_settings);
    }
}

//
// Helpers
//
uint32_t ManaInstance::get_api_version(const ManaVK::ManaInstance::ManaFeatures &features) const {
    uint32_t version = VK_API_VERSION_1_0;

    if (features.raytracing) {
        // TODO: Use a lower API version?
        version = VK_API_VERSION_1_3;
    }

    return version;
}
