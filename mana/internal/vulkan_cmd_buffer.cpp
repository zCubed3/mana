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

#include "vulkan_cmd_buffer.hpp"

#include <vulkan/vk_enum_string_helper.h>

#include <iostream>
#include <stdexcept>

using namespace ManaVK::Internal;

#define LOG_INLINE(args) std::cout << "[ManaVK::Internal::VulkanCmdBuffer]: "<< args
#define LOG(args) LOG_INLINE(args) << std::endl

VulkanCmdBuffer::VulkanCmdBuffer(const BufferConfig &config, VulkanQueue *owner)
    : owner(owner)
{
    if (config.vk_cmd_buffer == nullptr) {
        throw std::runtime_error("vk_cmd_buffer was nullptr! Please provide a valid command buffer handle!");
    }

    if (owner == nullptr) {
        throw std::runtime_error("owner was nullptr!");
    }

    this->vk_cmd_buffer = config.vk_cmd_buffer;
    this->vk_flags = config.vk_flags;
}

void VulkanCmdBuffer::begin(VulkanInstance *vulkan_instance) {
    if (vulkan_instance == nullptr) {
        throw std::runtime_error("vulkan_instance was nullptr!");
    }

    VkCommandBufferBeginInfo buffer_begin_info{};
    {
        buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        buffer_begin_info.flags = vk_flags;
    }

    VkResult result = vkBeginCommandBuffer(vk_cmd_buffer, &buffer_begin_info);

    if (result != VK_SUCCESS) {
        LOG("Error: vkBeginCommandBuffer failed with error code (" << string_VkResult(result) << ")");
        throw std::runtime_error("vkBeginCommandBuffer failed! Please check the log above for more info!");
    }
}

void VulkanCmdBuffer::end(VulkanInstance *vulkan_instance) {
    VkResult result = vkEndCommandBuffer(vk_cmd_buffer);

    if (result != VK_SUCCESS) {
        LOG("Error: vkEndCommandBuffer failed with error code (" << string_VkResult(result) << ")");
        throw std::runtime_error("vkEndCommandBuffer failed! Please check the log above for more info!");
    }
}
