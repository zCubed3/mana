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

#ifndef MANA_VULKAN_CMD_BUFFER_HPP
#define MANA_VULKAN_CMD_BUFFER_HPP

#include <vulkan/vulkan.h>

#include <vector>

namespace ManaVK::Internal {
    class VulkanInstance;
    class VulkanQueue;

    class VulkanCmdBuffer {
    public:
        struct BufferConfig {
            VkCommandBuffer vk_cmd_buffer = nullptr;
            VkCommandBufferUsageFlags vk_flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        };

        struct SubmitInfo {
            VkFence vk_fence;
            std::vector<VkSemaphore> vk_wait_semaphores;
            std::vector<VkSemaphore> vk_signal_semaphores;

            std::vector<VkPipelineStageFlags> vk_wait_flags {
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            };
        };

        struct PresentInfo {
            VulkanQueue *vulkan_queue_present = nullptr;
            VkSemaphore vk_semaphore_work_done = nullptr;

            VkSwapchainKHR vk_swapchain = nullptr;
            uint32_t frame_index = 0;
        };

    protected:
        VulkanQueue *owner = nullptr;
        VkCommandBuffer vk_cmd_buffer = nullptr;
        VkCommandBufferUsageFlags vk_flags;

    public:
        VulkanCmdBuffer(const BufferConfig &config, VulkanQueue *owner);

        void begin(VulkanInstance *vulkan_instance);
        void end(VulkanInstance *vulkan_instance);

        void submit(const SubmitInfo &info);
        void present(const PresentInfo &info);

        //
        // Getters
        //
        [[nodiscard]]
        VkCommandBuffer get_vk_cmd_buffer() const {
            return vk_cmd_buffer;
        }
    };
}

#endif//MANA_VULKAN_CMD_BUFFER_HPP
