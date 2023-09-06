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

#ifndef MANA_VULKAN_RENDER_TARGET_HPP
#define MANA_VULKAN_RENDER_TARGET_HPP

#include <vulkan/vulkan.h>

#include <memory>

namespace ManaVK::Internal {
    class VulkanInstance;
    class VulkanCmdBuffer;

    // TODO: Frames in flight
    class VulkanRenderTarget {
    public:
        [[nodiscard]]
        virtual std::shared_ptr<VulkanCmdBuffer> get_vulkan_cmd_buffer() const = 0;

        [[nodiscard]]
        virtual VkExtent2D get_vk_extent() const = 0;

        [[nodiscard]]
        virtual VkFramebuffer get_vk_framebuffer(VulkanInstance *vulkan_instance) const = 0;

        [[nodiscard]]
        virtual VkSemaphore get_vk_semaphore_work_done() const = 0;

        [[nodiscard]]
        virtual VkSemaphore get_vk_semaphore_image_ready() const = 0;

        [[nodiscard]]
        virtual VkFence get_vk_fence() const = 0;

        virtual void await_frame(VulkanInstance *vulkan_instance) const = 0;
        virtual void present_frame(VulkanInstance *vulkan_instance) const = 0;
    };
}

#endif//MANA_VULKAN_RENDER_TARGET_HPP
