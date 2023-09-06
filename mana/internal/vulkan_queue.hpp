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

#ifndef MANA_VULKAN_QUEUE_HPP
#define MANA_VULKAN_QUEUE_HPP

#include <vulkan/vulkan.h>

#include <cstdint>

namespace ManaVK::Internal {
    class VulkanCmdBuffer;

    class VulkanQueue {
    public:
        enum class Type {
            Graphics,
            Transfer,
            Present

            // TODO: Compute queue?
        };

    protected:
        Type type;
        uint32_t index;
        VkQueue vk_queue = nullptr;
        VkCommandPool vk_cmd_pool = nullptr;

    public:
        VulkanQueue() = delete;
        VulkanQueue(Type type, uint32_t index) {
            this->type = type;
            this->index = index;
        }

        void warm_queue(VkInstance vk_instance, VkDevice vk_device);

        VulkanCmdBuffer *allocate_cmd_buffer(VkDevice vk_device);

    public:
        [[nodiscard]]
        Type get_type() const {
            return type;
        }

        [[nodiscard]]
        uint32_t get_index() const {
            return index;
        }

        [[nodiscard]]
        VkQueue get_vk_queue() const {
            return vk_queue;
        }
    };
}

#endif//MANA_VULKAN_QUEUE_HPP
