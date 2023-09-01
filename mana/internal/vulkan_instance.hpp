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

namespace ManaVK::Internal {
    class VulkanWindow;

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
        // Layers are only optional when passed into filter_layers()
        // At initialization time they're no longer optional!
        class VulkanLayer : public VulkanAspect {
        public:
            VulkanLayer() = delete;
            VulkanLayer(std::string name, bool required) : VulkanAspect(name, required) {

            }
        };

        // Extensions are only optional when passed into filter_extensions()
        class VulkanExtension : public VulkanAspect {
        public:
            VulkanExtension() = delete;
            VulkanExtension(std::string name, bool required) : VulkanAspect(name, required) {

            }
        };

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

            std::vector<VulkanExtension> extensions;
            std::vector<VulkanLayer> layers;
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

        VulkanWindow *main_window = nullptr;

    public:
        //
        // Setup stages
        //
        void init_spawn_window(const WindowSettings& settings);
        void init_create_instance(const InstanceSettings& settings);

        //
        // Filtering functions
        //
        FilterResults<VulkanLayer> filter_layers(const std::vector<VulkanLayer>& list);
        FilterResults<VulkanExtension> filter_extensions(const std::vector<VulkanExtension>& list);

        //
        // Helpers
        //
        std::vector<VulkanExtension> get_sdl_extensions();

    protected:
        //
        // Instance creation
        //
        void create_vk_instance(const InstanceSettings& settings);
    };
}

#endif//MANA_VULKAN_INSTANCE_HPP
