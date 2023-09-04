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

#ifndef MANA_VULKAN_RENDER_PASS_BUILDER_HPP
#define MANA_VULKAN_RENDER_PASS_BUILDER_HPP

#include <vulkan/vulkan.h>

#include <vector>
#include <optional>
#include <memory>

namespace ManaVK::Internal {
    class VulkanRenderPass;

    class VulkanRenderPassBuilder {
    public:
        struct AttachmentInfo {
            VkImageLayout vk_layout_ref = VK_IMAGE_LAYOUT_UNDEFINED;
            VkFormat vk_format = VK_FORMAT_UNDEFINED;
            VkSampleCountFlagBits vk_samples = VK_SAMPLE_COUNT_1_BIT;

            VkAttachmentLoadOp vk_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            VkAttachmentStoreOp vk_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            VkAttachmentLoadOp vk_stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            VkAttachmentStoreOp vk_stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            VkImageLayout vk_layout_initial = VK_IMAGE_LAYOUT_UNDEFINED;
            VkImageLayout vk_layout_final = VK_IMAGE_LAYOUT_UNDEFINED;
        };

        // The user must be aware of their pushed indices
        // Even depth!
        struct SubpassInfo {
            VkPipelineBindPoint vk_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;

            std::vector<uint32_t> input_indices;
            std::vector<uint32_t> output_indices;

            std::optional<uint32_t> depth_index;
        };

        struct SubpassDependency {
            uint32_t src_subpass = VK_SUBPASS_EXTERNAL;
            uint32_t dst_subpass = VK_SUBPASS_EXTERNAL;

            VkPipelineStageFlags vk_stage_flags_src = 0;
            VkAccessFlags vk_access_flags_src = 0;

            VkPipelineStageFlags vk_stage_flags_dst = 0;
            VkAccessFlags vk_access_flags_dst = 0;

            VkDependencyFlags vk_dependency_flags = VK_DEPENDENCY_BY_REGION_BIT;
        };


    protected:
        struct Attachment {
            VkAttachmentDescription vk_description {};
            VkImageLayout vk_layout_ref = VK_IMAGE_LAYOUT_UNDEFINED;
        };

        std::vector<Attachment> color_attachments;
        std::optional<Attachment> depth_attachment;

        std::vector<SubpassInfo> subpasses;
        std::vector<SubpassDependency> dependencies;

    public:
        //
        // Methods
        //
        void push_subpass(const SubpassInfo& info);
        void push_dependency(const SubpassDependency& dependency);

        void push_color_attachment(const AttachmentInfo &info);
        void set_depth_attachment(const AttachmentInfo &info);

        std::shared_ptr<VulkanRenderPass> build(VkDevice vk_device);

    protected:
        //
        // Helpers
        //
        static Attachment decompose_attachment(const AttachmentInfo& info);
    };
}

#endif//MANA_VULKAN_RENDER_PASS_BUILDER_HPP
