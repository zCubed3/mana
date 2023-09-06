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

#include <mana/internal/vulkan_render_target.hpp>

namespace ManaVK::Internal {
    class VulkanInstance;
    class VulkanImage;
    class VulkanQueue;
    class VulkanCmdBuffer;

    class VulkanWindow : public VulkanRenderTarget {
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
            uint32_t frame_index;

            VkSwapchainKHR vk_swapchain = nullptr;
            std::vector<VkImage> vk_swapchain_images;
            std::vector<VkImageView> vk_swapchain_views;
            std::vector<VkFramebuffer> vk_framebuffers;

            std::unique_ptr<VulkanImage> vulkan_depth_image;
        };

    protected:
        SDL_Window *handle = nullptr;
        VkSurfaceKHR vk_surface = nullptr;
        VkSurfaceCapabilitiesKHR vk_capabilities;
        VkExtent2D vk_extent;

        VkSemaphore vk_semaphore_image_ready = nullptr;
        VkSemaphore vk_semaphore_work_done = nullptr;
        VkFence vk_fence = nullptr;

        std::shared_ptr<VulkanCmdBuffer> vulkan_cmd_buffer;
        std::unique_ptr<VulkanSwapchain> vulkan_swapchain;

        SwapchainConfig last_config;

    public:
        VulkanWindow(const std::string& name, int width, int height, bool resizable);

        void create_surface(VulkanInstance *vulkan_instance);
        void create_swapchain(VulkanInstance *vulkan_instance, const SwapchainConfig& config);
        void create_command_objects(VulkanInstance *vulkan_instance, VulkanQueue *vulkan_queue);

        void recreate_swapchain(VulkanInstance *vulkan_instance);

        void release_swapchain(VulkanInstance *vulkan_instance, std::unique_ptr<VulkanSwapchain> target = nullptr);

    public:
        [[nodiscard]]
        SDL_Window *get_handle() const {
           return handle;
        }

        [[nodiscard]]
        VkSurfaceKHR get_vk_surface() const {
           return vk_surface;
        }

        [[nodiscard]]
        VkExtent2D get_vk_extent() const override {
           return vk_extent;
        }

        [[nodiscard]]
        std::shared_ptr<VulkanCmdBuffer> get_vulkan_cmd_buffer() const override {
           return vulkan_cmd_buffer;
        }

        VkFramebuffer get_vk_framebuffer(VulkanInstance *vulkan_instance) const override;

        [[nodiscard]]
        VkSemaphore get_vk_semaphore_work_done() const override {
           return vk_semaphore_work_done;
        }

        [[nodiscard]]
        VkSemaphore get_vk_semaphore_image_ready() const override {
           return vk_semaphore_image_ready;
        }

        [[nodiscard]]
        VkFence get_vk_fence() const override {
           return vk_fence;
        }

        void await_frame(VulkanInstance *vulkan_instance) const override;

        void present_frame(VulkanInstance *vulkan_instance) const override;
    };
}

#endif//MANA_VULKAN_WINDOW_HPP
