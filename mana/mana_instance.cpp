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

#include <mana/mana_pipeline.hpp>
#include <mana/mana_render_pass.hpp>
#include <mana/mana_window.hpp>

#include <vulkan/vk_enum_string_helper.h>

#include <iostream>

using namespace ManaVK;

#define LOG_INLINE(args) std::cout << "[ManaVK::ManaInstance]: "<< args
#define LOG(args) LOG_INLINE(args) << std::endl

ManaInstance::ManaInstance(const ManaVK::ManaInstance::ManaConfig &config) {
    //
    // VulkanInstance bootstrapping
    //
    vulkan_instance = std::make_unique<Internal::VulkanInstance>();

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
            config.engine_version.rev,
            config.engine_version.major,
            config.engine_version.minor,
            config.engine_version.patch
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

            instance_settings.extensions = filtered.found;
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

            instance_settings.layers = filtered.found;
        }

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

            device_settings.extensions = filtered.found;
        }

        vulkan_instance->init_create_device(device_settings);
    }

    // Presentation initialization
    {
        Internal::VulkanInstance::PresentSettings present_settings;

        //
        // Find our preferred color formats
        //

        // Color formats
        // TODO: HDR? BGR? etc...
        std::vector<Internal::VulkanInstance::VulkanSurfaceFormat> requested_color_formats;
        {
            if (config.display_settings.srgb) {
                requested_color_formats.emplace_back(VK_FORMAT_B8G8R8A8_SRGB, VK_COLORSPACE_SRGB_NONLINEAR_KHR);
            } else {
                requested_color_formats.emplace_back(VK_FORMAT_B8G8R8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR);
            }
        }

        {
            auto filtered = vulkan_instance->filter_surface_formats(requested_color_formats);

            for (auto& format : filtered.missing) {
                if (config.debugging.verbose) {
                    LOG(
                        "Warning: Missing surface format '"
                        << string_VkFormat(format.get_vk_format())
                        << "' with colorspace '"
                        << string_VkColorSpaceKHR(format.get_vk_color_space())
                        << "'"
                    );
                }
            }

            if (filtered.found.empty()) {
                throw std::runtime_error("No supported surface formats found! This is a rare error and could indicate a driver problem / exotic hardware");
            }

            present_settings.color_formats = filtered.found;

            // TODO: No more jank!
            vk_format_default_color = present_settings.color_formats.back().get_vk_format();
        }

        // Depth formats
        std::vector<Internal::VulkanInstance::VulkanFormat> requested_depth_formats;
        {
            if (config.display_settings.precise_depth) {
                requested_depth_formats.emplace_back(VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
            }

            // Fallback even if precise depth doesn't exist
            // e.g. on iGPUs
            requested_depth_formats.emplace_back(VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
            requested_depth_formats.emplace_back(VK_FORMAT_D16_UNORM_S8_UINT, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        }

        {
            auto filtered = vulkan_instance->filter_formats(requested_depth_formats);

            for (auto& format : filtered.missing) {
                if (config.debugging.verbose) {
                    LOG(
                        "Warning: Missing format '"
                        << string_VkFormat(format.get_vk_format())
                        << "'"
                    );
                }
            }

            if (filtered.found.empty()) {
                throw std::runtime_error("No supported depth formats! This is a rare error and could indicate a driver problem / exotic hardware");
            }

            present_settings.depth_formats = filtered.found;

            // TODO: No more jank!
            vk_format_default_depth = present_settings.depth_formats.back().get_vk_format();
        }

        // Present modes
        std::vector<VkPresentModeKHR> requested_modes;
        {
            if (config.display_settings.vsync) {
                requested_modes.push_back(VK_PRESENT_MODE_FIFO_KHR);
                requested_modes.push_back(VK_PRESENT_MODE_FIFO_RELAXED_KHR);
            }

            // Fallback modes we always have!
            // TODO: Be more particular about missing modes?
            requested_modes.push_back(VK_PRESENT_MODE_MAILBOX_KHR);
            requested_modes.push_back(VK_PRESENT_MODE_IMMEDIATE_KHR);
        }

        {
            auto filtered = vulkan_instance->filter_present_modes(requested_modes);

            for (auto& mode : filtered.missing) {
                if (config.debugging.verbose) {
                    LOG("Warning: Missing present mode '" << string_VkPresentModeKHR(mode) << "'");
                }
            }

            if (filtered.found.empty()) {
                throw std::runtime_error("No supported present modes found! This is a rare error and could indicate a driver problem / exotic hardware");
            }

            present_settings.vk_present_modes = filtered.found;
        }

        // TODO: High precision / low precision color settings
        // TODO: Expose more defaults

        //
        // Pipeline building
        //
        this->mana_pipeline = config.mana_pipeline;

        {
            auto render_pass = mana_pipeline->build_render_pass(this);
            present_settings.vulkan_render_pass = render_pass->get_vulkan_render_pass();
        }

        //
        // Init
        //
        vulkan_instance->init_presentation(present_settings);
    }

    //
    // Post-bootstrap
    //
    main_window = std::make_shared<ManaWindow>(vulkan_instance->main_window);
}

//
// Methods
//
void ManaInstance::flush() {
    for (auto& func : release_queue) {
        func(this);
    }
}

void ManaInstance::enqueue_release(const std::function<void(ManaInstance *)> &func) {
    release_queue.emplace_back(func);
}

//
// Getters
//
int ManaInstance::get_vk_color_format(ManaVK::ManaColorFormat format) const {
    switch (format) {
        default:
            return mana_color_format_to_vk_format(format);

        case ManaColorFormat::Default:
            return vk_format_default_color;
    }
}

int ManaInstance::get_vk_depth_format(ManaVK::ManaDepthFormat format) const {
    switch (format) {
        default:
            return mana_depth_format_to_vk_format(format);

        case ManaDepthFormat::Default:
            return vk_format_default_depth;
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
