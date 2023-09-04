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

#ifndef MANA_VULKAN_INSTANCE_HPP
#define MANA_VULKAN_INSTANCE_HPP

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <memory>

namespace ManaVK::Internal {
    class VulkanWindow;
    class VulkanQueue;
    class VulkanRenderPass;

    class VulkanInstance {
    protected:
        // An aspect of Vulkan that can enabled / disabled by name
        class VulkanAspect {
        protected:
            std::string name;
            bool required;

        public:
            VulkanAspect() = delete;
            VulkanAspect(std::string name, bool required) {
                this->name = name;
                this->required = required;
            }

            [[nodiscard]]
            const char* get_cstr() const {
                return name.c_str();
            }

            [[nodiscard]]
            std::string get_name() const {
                return name;
            }

            [[nodiscard]]
            bool is_required() const {
                return required;
            }
        };

    public:
        // Aspects are not optional
        // Their purpose is to help classify data that is filtered!

        class VulkanLayer : public VulkanAspect {
        public:
            VulkanLayer() = delete;
            VulkanLayer(std::string name, bool required) : VulkanAspect(name, required) {

            }
        };

        class VulkanInstanceExtension : public VulkanAspect {
        public:
            VulkanInstanceExtension() = delete;
            VulkanInstanceExtension(std::string name, bool required) : VulkanAspect(name, required) {

            }
        };

        class VulkanDeviceExtension : public VulkanAspect {
        public:
            VulkanDeviceExtension() = delete;
            VulkanDeviceExtension(std::string name, bool required) : VulkanAspect(name, required) {

            }
        };

        class VulkanFormat {
        protected:
            VkFormat vk_format;
            VkImageTiling vk_tiling;
            VkFormatFeatureFlags vk_feature_flags;

        public:
            VulkanFormat(VkFormat vk_format, VkImageTiling vk_tiling, VkFormatFeatureFlags vk_feature_flags) {
                this->vk_format = vk_format;
                this->vk_tiling = vk_tiling;
                this->vk_feature_flags = vk_feature_flags;
            }

            [[nodiscard]]
            VkFormat get_vk_format() const {
                return vk_format;
            }

            [[nodiscard]]
            VkImageTiling get_vk_tiling() const {
                return vk_tiling;
            }

            [[nodiscard]]
            VkFormatFeatureFlags get_vk_feature_flags() const {
                return vk_feature_flags;
            }
        };

        class VulkanSurfaceFormat : public VulkanFormat {
        protected:
            VkColorSpaceKHR vk_color_space;

        public:
            VulkanSurfaceFormat() = delete;
            VulkanSurfaceFormat(VkFormat vk_format, VkColorSpaceKHR vk_color_space) : VulkanFormat(vk_format, {}, {}) {
                this->vk_color_space = vk_color_space;
            }

            [[nodiscard]]
            VkColorSpaceKHR get_vk_color_space() const {
                return vk_color_space;
            }
        };

    public:
        struct WindowSettings {
            std::string title = "ManaVK Window";
            int width = 1280;
            int height = 720;
        };

        struct InstanceSettings {
            std::string engine_name = "ManaVK";
            std::string app_name = "ManaVK Application";

            uint32_t engine_version = VK_MAKE_VERSION(0, 0, 0);
            uint32_t app_version = VK_MAKE_VERSION(0, 0, 0);
            uint32_t api_version = VK_API_VERSION_1_0;

            std::vector<VulkanInstanceExtension> extensions;
            std::vector<VulkanLayer> layers;
        };

        struct GPUPreferences {
            bool need_graphics_queue = true;
            bool need_transfer_queue = true;
            bool need_present_queue = true;

            VkPhysicalDeviceFeatures required_features{};
        };

        struct DeviceSettings {
            std::vector<VulkanDeviceExtension> extensions;
        };

        struct PresentSettings {
            std::vector<VulkanSurfaceFormat> color_formats;

            // If empty, the window has no depth buffering
            std::vector<VulkanFormat> depth_formats;

            std::vector<VkPresentModeKHR> vk_present_modes;

            // The user must create their own render pass at this point
            // This is because an engine might use either deferred or forward rendering!
            // Or some exotic pipelines like Forward+, Deferred+, etc...
            std::shared_ptr<VulkanRenderPass> vulkan_render_pass = nullptr;
        };

        //
        // Filtering
        //
    public:
        template<class T>
        struct FilterResults {
            std::vector<T> found {};
            std::vector<T> missing {};
        };

    protected:
        template<class T>
        FilterResults<T> filter(std::vector<T> list, std::function<bool(VulkanInstance*, const T&)> predicate) {
            auto results = FilterResults<T>();

            for (auto& elem : list) {
                if (predicate(this, elem)) {
                    results.found.emplace_back(elem);
                } else {
                    results.missing.emplace_back(elem);
                }
            }

            return results;
        }

    public:
        VkInstance vk_instance = nullptr;
        VkDevice vk_device = nullptr;
        VkPhysicalDevice vk_gpu = nullptr;
        VmaAllocator vma_allocator = nullptr;
        VkDescriptorPool vk_descriptor_pool = nullptr;

        VulkanWindow *main_window = nullptr;

        // TODO: Do we ever need multiple queues of the same type?
        VulkanQueue* queue_graphics = nullptr;
        VulkanQueue* queue_transfer = nullptr;
        VulkanQueue* queue_present = nullptr;

        std::optional<VulkanSurfaceFormat> vulkan_color_format;
        std::optional<VulkanFormat> vulkan_depth_format;
        VkPresentModeKHR vk_present_mode = VK_PRESENT_MODE_MAX_ENUM_KHR;

    public:
        //
        // Setup stages
        //
        void init_spawn_window(const WindowSettings& settings);
        void init_create_instance(const InstanceSettings& settings);
        void init_find_gpu(const GPUPreferences& prefs);
        void init_create_device(const DeviceSettings& settings);
        void init_presentation(const PresentSettings& settings);

        //
        // Filtering functions
        //
        FilterResults<VulkanLayer> filter_layers(const std::vector<VulkanLayer>& list);
        FilterResults<VulkanInstanceExtension> filter_instance_extensions(const std::vector<VulkanInstanceExtension>& list);
        FilterResults<VulkanDeviceExtension> filter_device_extensions(const std::vector<VulkanDeviceExtension>& list);
        FilterResults<VulkanFormat> filter_formats(const std::vector<VulkanFormat>& list);
        FilterResults<VulkanSurfaceFormat> filter_surface_formats(const std::vector<VulkanSurfaceFormat>& list);
        FilterResults<VkPresentModeKHR> filter_present_modes(const std::vector<VkPresentModeKHR>& list);

        //
        // Helpers
        //
        std::vector<VulkanInstanceExtension> get_sdl_extensions();

        //
        // Getters
        //
        [[nodiscard]]
        VkInstance get_vk_instance() const {
            return vk_instance;
        }

        [[nodiscard]]
        VkDevice get_vk_device() const {
            return vk_device;
        }

        [[nodiscard]]
        VkPhysicalDevice get_vk_gpu() const {
            return vk_gpu;
        }

        [[nodiscard]]
        VulkanQueue *get_queue_graphics() const {
            return queue_graphics;
        }

        [[nodiscard]]
        VulkanQueue *get_queue_present() const {
            return queue_present;
        }

        [[nodiscard]]
        VulkanQueue *get_queue_transfer() const {
            return queue_transfer;
        }
    };
}

#endif//MANA_VULKAN_INSTANCE_HPP
