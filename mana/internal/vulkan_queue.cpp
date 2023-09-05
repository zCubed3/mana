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

#include "vulkan_queue.hpp"

#include <mana/internal/vulkan_cmd_buffer.hpp>

#include <vulkan/vk_enum_string_helper.h>

#include <stdexcept>
#include <iostream>

using namespace ManaVK;

#define LOG_INLINE(args) std::cout << "[ManaVK::Internal::VulkanQueue]: "<< args
#define LOG(args) LOG_INLINE(args) << std::endl

void Internal::VulkanQueue::warm_queue(VkInstance vk_instance, VkDevice vk_device) {
    if (vk_queue != nullptr) {
        throw std::runtime_error("Queue was already warmed!");
    }

    if (vk_instance == nullptr) {
        throw std::runtime_error("vk_instance was nullptr!");
    }

    if (vk_device == nullptr) {
        throw std::runtime_error("vk_device was nullptr!");
    }

    vkGetDeviceQueue(vk_device, index, 0, &vk_queue);

    if (vk_queue == nullptr) {
        throw std::runtime_error("vkGetDeviceQueue failed!");
    }

    uint32_t flags;
    switch (type) {
        default:
            flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            break;

        case Type::Transfer:
            flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            break;
    }

    VkCommandPoolCreateInfo pool_create_info{};

    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.flags = flags;
    pool_create_info.queueFamilyIndex = index;

    VkResult result = vkCreateCommandPool(vk_device, &pool_create_info, nullptr, &vk_cmd_pool);

    if (result != VK_SUCCESS) {
        LOG("Error: vkCreateCommandPool failed with error code (" << string_VkResult(result) << ")");
        throw std::runtime_error("vkCreateCommandPool failed! Please check the log above for more info!");
    }
}

Internal::VulkanCmdBuffer *Internal::VulkanQueue::allocate_cmd_buffer(VkDevice vk_device) {
    if (vk_device == nullptr) {
        throw std::runtime_error("vk_device was nullptr!");
    }

    VkCommandBufferAllocateInfo alloc_info{};
    {
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

        alloc_info.commandPool = vk_cmd_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = 1;
    }

    VkCommandBuffer vk_buffer = nullptr;
    VkResult result = vkAllocateCommandBuffers(vk_device, &alloc_info, &vk_buffer);

    if (result != VK_SUCCESS) {
        LOG("vkAllocateCommandBuffers failed with error code (" << string_VkResult(result) << ")");
        throw std::runtime_error("vkAllocateCommandBuffers failed! Please check the log above for more info!");
    }

    VulkanCmdBuffer::BufferConfig config;
    {
        config.vk_cmd_buffer = vk_buffer;
        // TODO: Change flags (when necessary)
    }

    return new VulkanCmdBuffer(config, this);
}