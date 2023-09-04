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

#include "vulkan_render_pass_builder.hpp"

#include <mana/internal/vulkan_render_pass.hpp>

#include <vulkan/vk_enum_string_helper.h>

#include <stdexcept>
#include <iostream>

using namespace ManaVK;

#define LOG_INLINE(args) std::cout << "[ManaVK::Internal::VulkanRenderPassBuilder]: "<< args
#define LOG(args) LOG_INLINE(args) << std::endl

void Internal::VulkanRenderPassBuilder::push_subpass(const SubpassInfo &info) {
    subpasses.emplace_back(info);
}

void Internal::VulkanRenderPassBuilder::push_dependency(const SubpassDependency &dependency) {
    dependencies.emplace_back(dependency);
}

void Internal::VulkanRenderPassBuilder::push_color_attachment(const AttachmentInfo &info) {
    color_attachments.emplace_back(decompose_attachment(info));
}

void Internal::VulkanRenderPassBuilder::set_depth_attachment(const AttachmentInfo &info) {
    depth_attachment = decompose_attachment(info);
}

std::shared_ptr<Internal::VulkanRenderPass> Internal::VulkanRenderPassBuilder::build(VkDevice vk_device) {
    if (vk_device == nullptr) {
        throw std::runtime_error("vk_device was nullptr!");
    }

    //
    // Reference building
    //
    std::vector<Attachment> attachments;
    {
        attachments.insert(attachments.end(), color_attachments.begin(), color_attachments.end());

        if (depth_attachment.has_value()) {
            attachments.push_back(depth_attachment.value());
        }
    }

    std::vector<VkAttachmentDescription> vk_attachments;
    std::vector<VkAttachmentReference> vk_attachment_refs;
    {
        uint32_t index = 0;

        for (auto attachment : attachments) {
            VkAttachmentReference vk_attachment_ref {};

            vk_attachment_ref.attachment = index++;
            vk_attachment_ref.layout = attachment.vk_layout_ref;

            vk_attachments.emplace_back(attachment.vk_description);
            vk_attachment_refs.emplace_back(vk_attachment_ref);
        }
    }

    //
    // Subpass building
    //
    std::vector<std::unique_ptr<VkAttachmentReference[]>> jagged_inputs;
    std::vector<std::unique_ptr<VkAttachmentReference[]>> jagged_outputs;
    {
        for (const auto& subpass : subpasses) {
            auto inputs = std::make_unique<VkAttachmentReference[]>(subpass.input_indices.size());
            auto outputs = std::make_unique<VkAttachmentReference[]>(subpass.output_indices.size());

            uint32_t index = 0;

            for (auto input : subpass.input_indices) {
                inputs[index++] = vk_attachment_refs[input];
            }

            index = 0;

            for (auto output : subpass.output_indices) {
                outputs[index++] = vk_attachment_refs[output];
            }

            jagged_inputs.emplace_back(std::move(inputs));
            jagged_outputs.emplace_back(std::move(outputs));
        }
    }

    std::vector<VkSubpassDescription> vk_subpasses;
    {
        uint32_t index = 0;

        for (const auto& subpass : subpasses) {
            VkSubpassDescription vk_subpass{};
            {
                vk_subpass.pipelineBindPoint = subpass.vk_bind_point;

                vk_subpass.colorAttachmentCount = subpass.output_indices.size();
                vk_subpass.pColorAttachments = jagged_outputs[index].get();

                vk_subpass.inputAttachmentCount = subpass.input_indices.size();
                vk_subpass.pInputAttachments = jagged_inputs[index].get();

                if (subpass.depth_index.has_value()) {
                    vk_subpass.pDepthStencilAttachment = vk_attachment_refs.data() + subpass.depth_index.value();
                }
            }

            index++;
            vk_subpasses.emplace_back(vk_subpass);
        }
    }

    std::vector<VkSubpassDependency> vk_dependencies;
    {
        for (const auto& dependency : dependencies) {
            VkSubpassDependency vk_dependency{};
            {
                vk_dependency.dependencyFlags = dependency.vk_dependency_flags;

                vk_dependency.srcAccessMask = dependency.vk_access_flags_src;
                vk_dependency.dstAccessMask = dependency.vk_access_flags_dst;

                vk_dependency.srcStageMask = dependency.vk_stage_flags_src;
                vk_dependency.dstStageMask = dependency.vk_stage_flags_dst;

                vk_dependency.srcSubpass = dependency.src_subpass;
                vk_dependency.dstSubpass = dependency.dst_subpass;
            }

            vk_dependencies.emplace_back(vk_dependency);
        }
    }

    //
    // Render pass creation
    //
    VkRenderPassCreateInfo render_pass_create_info{};
    {
        render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

        render_pass_create_info.attachmentCount = static_cast<uint32_t>(vk_attachments.size());
        render_pass_create_info.pAttachments = vk_attachments.data();

        render_pass_create_info.subpassCount = static_cast<uint32_t>(vk_subpasses.size());
        render_pass_create_info.pSubpasses = vk_subpasses.data();

        render_pass_create_info.dependencyCount = static_cast<uint32_t>(vk_dependencies.size());
        render_pass_create_info.pDependencies = vk_dependencies.data();
    }

    VkRenderPass vk_render_pass = nullptr;
    VkResult result = vkCreateRenderPass(vk_device, &render_pass_create_info, nullptr, &vk_render_pass);

    if (result != VK_SUCCESS) {
        LOG("vkCreateRenderPass failed with error code (" << string_VkResult(result) << ")");
        throw std::runtime_error("vkCreateRenderPass failed! Please check the log above for more info!");
    }

    return std::make_shared<VulkanRenderPass>(vk_render_pass);
}

//
// Helpers
//
Internal::VulkanRenderPassBuilder::Attachment Internal::VulkanRenderPassBuilder::decompose_attachment(const AttachmentInfo &info) {
    Attachment attachment;

    VkAttachmentDescription vk_description{};
    {
        vk_description.format = info.vk_format;
        vk_description.samples = info.vk_samples;

        vk_description.loadOp = info.vk_load_op;
        vk_description.storeOp = info.vk_store_op;

        vk_description.stencilLoadOp = info.vk_stencil_load_op;
        vk_description.stencilStoreOp = info.vk_stencil_store_op;

        vk_description.initialLayout = info.vk_layout_initial;
        vk_description.finalLayout = info.vk_layout_final;
    }

    attachment.vk_description = vk_description;
    attachment.vk_layout_ref = info.vk_layout_ref;

    return attachment;
}
