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

#ifndef MANA_MANA_INSTANCE_HPP
#define MANA_MANA_INSTANCE_HPP

#include <memory>
#include <string>
#include <vector>
#include <functional>

#include <mana/mana_enums.hpp>

namespace ManaVK::Internal {
    class VulkanInstance;
}

namespace ManaVK::Builders {
    class ManaRenderPassBuilder;
}

namespace ManaVK {
    class ManaWindow;
    class ManaPipeline;

    // Mana is generic, but tailored towards game usage
    // This means a transfer, graphics, and present queue are all allocated by default
    // Mana will handle creating windows for you
    // Initializing the instance provides you with the main window used for Vulkan creation
    class ManaInstance {
    public:
        struct ManaFeatures {
            // By default, basic raster is REQUIRED!

            // TODO: Implement raytracing
            bool raytracing = false;
        };

        struct ManaDebugging {
            bool enable_khronos_layer = false;
            bool verbose = false;
        };

        struct ManaVersion {
            uint32_t major = 1;
            uint32_t minor = 0;
            uint32_t patch = 0;
            uint32_t rev = 0;

            ManaVersion(uint32_t major, uint32_t minor, uint32_t patch, uint32_t rev) {
                this->major = major;
                this->minor = minor;
                this->patch = patch;
                this->rev = rev;
            }
        };

        struct ManaWindowSettings {
            std::string title = "ManaVK Window";
            int width = 1280;
            int height = 720;
        };

        struct ManaDisplaySettings {
            bool vsync = true;
            bool srgb = false;
            bool precise_depth = true;
        };

        struct ManaConfig {
            ManaFeatures features;
            ManaDebugging debugging;
            ManaWindowSettings window_settings;
            ManaDisplaySettings display_settings;

            std::shared_ptr<ManaPipeline> mana_pipeline;

            std::string engine_name = "ManaVK Engine";
            ManaVersion engine_version = ManaVersion(1, 0, 0, 0);

            std::string app_name = "ManaVK Application";
            ManaVersion app_version = ManaVersion(1, 0, 0, 0);
        };

    protected:
        std::shared_ptr<ManaPipeline> mana_pipeline;

        std::shared_ptr<Internal::VulkanInstance> vulkan_instance = nullptr;

        std::shared_ptr<ManaWindow> main_window = nullptr;
        std::vector<std::shared_ptr<ManaWindow>> child_windows;

        std::vector<std::function<void(ManaInstance*)>> release_queue;

        int vk_format_default_color;
        int vk_format_default_depth;

    public:
        // Bootstraps the user through initial setup without the user having to touch Vulkan once!
        ManaInstance(const ManaConfig& config);

        //
        // Methods
        //

        // Usually called before any rendering is done
        // This will process the release queue
        // But will also process the transfer queue
        void flush();


        // Queues a release function
        void enqueue_release(const std::function<void(ManaInstance*)> &func);

    public:
        //
        // Getters
        //
        [[nodiscard]]
        std::shared_ptr<ManaWindow> get_main_window() const {
            return main_window;
        }

        [[nodiscard]]
        std::shared_ptr<Internal::VulkanInstance> get_vulkan_instance() const {
            return vulkan_instance;
        }

        int get_vk_color_format(ManaColorFormat format) const;
        int get_vk_depth_format(ManaDepthFormat format) const;

    protected:
        //
        // Helpers
        //
        uint32_t get_api_version(const ManaFeatures& features) const;
    };
}

#endif//MANA_MANA_INSTANCE_HPP
