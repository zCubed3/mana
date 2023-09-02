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

#ifndef MANA_VULKAN_WINDOW_HPP
#define MANA_VULKAN_WINDOW_HPP

#include <SDL.h>
#include <vulkan/vulkan.h>

#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace ManaVK::Internal {
    class VulkanInstance;
    class VulkanImage;

    class VulkanWindow {
    public:
        struct SwapchainConfig {
            VkFormat vk_format_color;
            VkColorSpaceKHR vk_color_space;
            VkPresentModeKHR vk_present_mode;

            VkRenderPass vk_render_pass = nullptr;

            std::optional<VkFormat> vk_format_depth;
        };

        // TODO: Should this not be part of the window?
        struct VulkanSwapchain {
            VkSwapchainKHR vk_swapchain = nullptr;
            std::vector<VkImage> vk_swapchain_images;
            std::vector<VkImageView> vk_swapchain_views;
            std::vector<VkFramebuffer> vk_framebuffers;

            std::shared_ptr<VulkanImage> vulkan_depth_image;
        };

    protected:
        SDL_Window *handle = nullptr;
        VkSurfaceKHR vk_surface = nullptr;
        VkSurfaceCapabilitiesKHR vk_capabilities;
        VkExtent2D vk_extent;

        // Used for faster recreation of the swapchain (e.g. resizing)
        SwapchainConfig last_config;
        std::unique_ptr<VulkanSwapchain> vulkan_swapchain;

    public:
        VulkanWindow(const std::string& name, int width, int height);

        void create_surface(VkInstance vk_instance);
        void create_swapchain(VulkanInstance *vulkan_instance, const SwapchainConfig& config);

        void release_swapchain(VkDevice vk_device, std::unique_ptr<VulkanSwapchain> target = nullptr);

    public:
        [[nodiscard]]
        SDL_Window *get_handle() const {
           return handle;
        }

        [[nodiscard]]
        VkSurfaceKHR get_vk_surface() const {
           return vk_surface;
        }
    };
}

#endif//MANA_VULKAN_WINDOW_HPP
