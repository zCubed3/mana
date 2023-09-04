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

#include "vulkan_window.hpp"

#include <mana/internal/vulkan_instance.hpp>
#include <mana/internal/vulkan_queue.hpp>
#include <mana/internal/vulkan_image.hpp>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <SDL.h>
#include <SDL_vulkan.h>

#include <stdexcept>
#include <iostream>

#define LOG_INLINE(args) std::cout << "[ManaVK::Internal::VulkanWindow]: "<< args
#define LOG(args) LOG_INLINE(args) << std::endl

using namespace ManaVK;

// TODO: Window position?
Internal::VulkanWindow::VulkanWindow(const std::string &name, int width, int height) {
    handle = SDL_CreateWindow(
        name.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_VULKAN
    );
}

void Internal::VulkanWindow::create_surface(VkInstance vk_instance) {
    if (vk_instance == nullptr) {
        throw std::runtime_error("vk_instance was nullptr!");
    }

    if (!SDL_Vulkan_CreateSurface(handle, vk_instance, &vk_surface)) {
        throw std::runtime_error("SDL_Vulkan_CreateSurface failed!");
    }
}

void Internal::VulkanWindow::create_swapchain(VulkanInstance *vulkan_instance, const SwapchainConfig& config) {
    if (handle == nullptr) {
        throw std::runtime_error("SDL window handle was nullptr!");
    }

    if (vulkan_instance == nullptr) {
        throw std::runtime_error("vulkan_instance was nullptr!");
    }

    if (config.vk_render_pass == nullptr) {
        throw std::runtime_error("vk_render_pass was nullptr! You can't create a framebuffer without a render pass!");
    }

    //
    // Update our view data
    //
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan_instance->get_vk_gpu(), vk_surface, &vk_capabilities);

    // TODO: Window supersampling

    // High dpi aware extent
    if (vk_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        vk_extent = vk_capabilities.currentExtent;
    } else {
        int width, height;
        SDL_Vulkan_GetDrawableSize(handle, &width, &height);

        VkExtent2D actual_extent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)};

        actual_extent.width = std::min(std::max(actual_extent.width, vk_capabilities.minImageExtent.width), vk_capabilities.maxImageExtent.width);
        actual_extent.height = std::min(std::max(actual_extent.height, vk_capabilities.minImageExtent.height), vk_capabilities.maxImageExtent.height);

        vk_extent = actual_extent;
    }

    // How many frames can the window render?
    uint32_t image_count = vk_capabilities.minImageCount + 1;
    if (vk_capabilities.maxImageCount > 0 && image_count > vk_capabilities.maxImageCount) {
        image_count = vk_capabilities.maxImageCount;
    }

    //
    // Vulkan swapchain creation
    //
    auto new_swapchain = std::make_unique<VulkanSwapchain>();
    {
        VkSwapchainCreateInfoKHR create_info{};

        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

        create_info.surface = vk_surface;
        create_info.minImageCount = image_count;
        create_info.imageFormat = config.vk_format_color;
        create_info.imageColorSpace = config.vk_color_space;
        create_info.imageExtent = vk_extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        create_info.preTransform = vk_capabilities.currentTransform;
        // TODO: Allow alpha (for overlays)
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        create_info.presentMode = config.vk_present_mode;
        create_info.clipped = VK_TRUE;

        if (vulkan_swapchain != nullptr) {
            create_info.oldSwapchain = vulkan_swapchain->vk_swapchain;
        }

        VulkanQueue *queue_graphics = vulkan_instance->get_queue_graphics();
        VulkanQueue *queue_present = vulkan_instance->get_queue_present();

        std::vector<uint32_t> device_queues = {
                queue_graphics->get_index(),
                queue_present->get_index()};

        if (queue_graphics->get_index() != queue_present->get_index()) {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = static_cast<uint32_t>(device_queues.size());
            create_info.pQueueFamilyIndices = device_queues.data();
        } else {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 0;
            create_info.pQueueFamilyIndices = nullptr;
        }

        VkResult result = vkCreateSwapchainKHR(vulkan_instance->get_vk_device(), &create_info, nullptr, &new_swapchain->vk_swapchain);

        if (result != VK_SUCCESS) {
            LOG("vkCreateSwapchainKHR failed with error code (" << string_VkResult(result) << ")");
            throw std::runtime_error("vkCreateSwapchainKHR failed! Please check the log above for more info!");
        }
    }

    //
    // Depth image creation
    //
    if (config.vk_format_depth.has_value()) {
        Internal::VulkanImage::ImageSettings image_settings;

        image_settings.vk_format = config.vk_format_depth.value();
        image_settings.vk_extent = {vk_extent.width, vk_extent.height, 1};

        // TODO: Optional stenciling?
        image_settings.vk_usage_flags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        image_settings.vk_aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

        // TODO: Implement VulkaImage!
        //new_swapchain->vulkan_depth_image = std::make_shared<VulkanImage>();
    }

    //
    // Swapchain image fetching
    //
    {
        vkGetSwapchainImagesKHR(vulkan_instance->get_vk_device(), new_swapchain->vk_swapchain, &image_count, nullptr);
        new_swapchain->vk_swapchain_images.resize(image_count);
        vkGetSwapchainImagesKHR(vulkan_instance->get_vk_device(), new_swapchain->vk_swapchain, &image_count, new_swapchain->vk_swapchain_images.data());
    }

    //
    // Creating views
    //
    {
        new_swapchain->vk_swapchain_views.resize(image_count);
        for (uint32_t i = 0; i < image_count; i++) {
            VkImageViewCreateInfo view_create_info{};
            view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

            view_create_info.image = new_swapchain->vk_swapchain_images[i];
            view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_create_info.format = config.vk_format_color;

            view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            view_create_info.subresourceRange.baseMipLevel = 0;
            view_create_info.subresourceRange.levelCount = 1;
            view_create_info.subresourceRange.baseArrayLayer = 0;
            view_create_info.subresourceRange.layerCount = 1;

            VkResult result = vkCreateImageView(vulkan_instance->get_vk_device(), &view_create_info, nullptr, &new_swapchain->vk_swapchain_views[i]);

            if (result != VK_SUCCESS) {
                LOG("vkCreateImageView failed with error code (" << string_VkResult(result) << ")");
                throw std::runtime_error("vkCreateImageView failed! Please check the log above for more info!");
            }
        }
    }

    //
    // Creating framebuffers
    //
    {
        new_swapchain->vk_framebuffers.resize(image_count);

        for (uint32_t i = 0; i < image_count; i++) {
            std::vector<VkImageView> attachments = {
                new_swapchain->vk_swapchain_views[i],
            };

            if (new_swapchain->vulkan_depth_image != nullptr) {
                attachments.push_back(new_swapchain->vulkan_depth_image->get_vk_view());
            }

            VkFramebufferCreateInfo framebuffer_create_info{};
            {
                framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebuffer_create_info.renderPass = config.vk_render_pass;
                framebuffer_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
                framebuffer_create_info.pAttachments = attachments.data();
                framebuffer_create_info.width = vk_extent.width;
                framebuffer_create_info.height = vk_extent.height;
                framebuffer_create_info.layers = 1;
            }

            VkResult result = vkCreateFramebuffer(vulkan_instance->get_vk_device(), &framebuffer_create_info, nullptr, &new_swapchain->vk_framebuffers[i]);

            if (result != VK_SUCCESS) {
                LOG("vkCreateFramebuffer failed with error code (" << string_VkResult(result) << ")");
                throw std::runtime_error("vkCreateFramebuffer failed! Please check the log above for more info!");
            }
        }
    }

    //
    // Release and replace previous swapchain info
    //
    if (vulkan_swapchain != nullptr) {
        release_swapchain(vulkan_instance->get_vk_device(), std::move(vulkan_swapchain));
    }

    vulkan_swapchain = std::move(new_swapchain);
    last_config = config;
}

void Internal::VulkanWindow::release_swapchain(VkDevice vk_device, std::unique_ptr<VulkanSwapchain> target) {
    auto actual = std::move(target);

    // If no alternative, use our own
    if (actual == nullptr) {
        actual = std::move(vulkan_swapchain);
    }

    // If all are freed prior, something has gone seriously wrong!
    if (actual == nullptr) {
        throw std::runtime_error("No swapchain to free! Something has gone seriously wrong!");
    }

    //
    // Deallocation (immediate, you're responsible for the syncing)
    //
    if (actual->vk_swapchain != nullptr) {
        vkDestroySwapchainKHR(vk_device, actual->vk_swapchain, nullptr);

        // TODO: Destroy depth images

        for (auto vk_view: actual->vk_swapchain_views) {
            vkDestroyImageView(vk_device, vk_view, nullptr);
        }
    }
}
