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

#include "vulkan_render_pass.hpp"

#include <mana/internal/vulkan_cmd_buffer.hpp>
#include <mana/internal/vulkan_render_target.hpp>

#include <stdexcept>

using namespace ManaVK::Internal;

VulkanRenderPass::VulkanRenderPass(const VulkanRenderPass::PassConfig &config) {
    this->vk_render_pass = config.vk_render_pass;
    this->attachment_count = config.attachment_count;
    this->depth_index = config.depth_index;
}

// TODO: RenderPass begin without render target?
void VulkanRenderPass::begin(VulkanInstance *vulkan_instance, const StateInfo &info) {

    if (info.vulkan_render_target == nullptr) {
        throw std::runtime_error("vulkan_render_target was nullptr!");
    }

    if (vulkan_instance == nullptr) {
        throw std::runtime_error("vulkan_instance was nullptr!");
    }

    if (vk_render_pass == nullptr) {
        throw std::runtime_error("vk_render_pass was nullptr!");
    }

    //
    // Begin the render pass
    //
    auto vk_cmd_buffer = info.vulkan_render_target->get_vulkan_cmd_buffer()->get_vk_cmd_buffer();

    VkRenderPassBeginInfo begin_info {};
    {
        begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

        begin_info.renderPass = vk_render_pass;
        begin_info.framebuffer = info.vulkan_render_target->get_vk_framebuffer(vulkan_instance);

        if (info.vk_render_offset) {
            begin_info.renderArea.offset = info.vk_render_offset.value();
        } else {
            begin_info.renderArea.offset = {0, 0};
        }

        if (info.vk_render_area) {
            begin_info.renderArea.extent = info.vk_render_area.value();
        } else {
            begin_info.renderArea.extent = info.vulkan_render_target->get_vk_extent();
        }

        begin_info.clearValueCount = info.vk_clear_values.size();
        begin_info.pClearValues = info.vk_clear_values.data();
    }

    // TODO: Allow multiple command buffers for multiple subpasses?
    vkCmdBeginRenderPass(vk_cmd_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRenderPass::end(const StateInfo &info) {
    if (info.vulkan_render_target == nullptr) {
        throw std::runtime_error("vulkan_cmd_buffer was nullptr!");
    }

    vkCmdEndRenderPass(info.vulkan_render_target->get_vulkan_cmd_buffer()->get_vk_cmd_buffer());
}

void VulkanRenderPass::release(VkDevice vk_device) {
    if (vk_render_pass != nullptr) {
        if (vk_device == nullptr) {
            throw std::runtime_error("vk_device was nullptr!");
        }

        vkDestroyRenderPass(vk_device, vk_render_pass, nullptr);
    }
}
