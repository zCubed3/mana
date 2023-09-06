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

#ifndef MANA_VULKAN_RENDER_PASS_HPP
#define MANA_VULKAN_RENDER_PASS_HPP

#include <vulkan/vulkan.h>

#include <optional>
#include <vector>

namespace ManaVK::Internal {
    class VulkanCmdBuffer;
    class VulkanRenderPass;
    class VulkanRenderTarget;
    class VulkanInstance;

    class VulkanRenderPass {
    public:
        struct PassConfig {
            VkRenderPass vk_render_pass = nullptr;

            uint32_t attachment_count;
            std::optional<uint32_t> depth_index;
        };

        struct StateInfo {
            VulkanRenderTarget *vulkan_render_target = nullptr;

            std::vector<VkClearValue> vk_clear_values;
            std::optional<VkExtent2D> vk_render_area;
            std::optional<VkOffset2D> vk_render_offset;
        };

    protected:
        VkRenderPass vk_render_pass = nullptr;
        uint32_t attachment_count = 0;
        std::optional<uint32_t> depth_index;

    public:
        VulkanRenderPass(const PassConfig &config);

        void begin(VulkanInstance *vulkan_instance, const StateInfo &info);
        void end(const StateInfo &info);

        void release(VkDevice vk_device);

        [[nodiscard]]
        VkRenderPass get_vk_render_pass() const {
            return vk_render_pass;
        }

        [[nodiscard]]
        bool has_depth() const {
            return depth_index.has_value();
        }
    };
}


#endif//MANA_VULKAN_RENDER_PASS_HPP
