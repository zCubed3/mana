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

#include "mana_render_pass.hpp"

#include <mana/internal/vulkan_render_pass.hpp>
#include <mana/internal/vulkan_instance.hpp>
#include <mana/internal/vulkan_render_target.hpp>

#include <mana/mana_instance.hpp>

#include <stdexcept>

using namespace ManaVK;

ManaVK::ManaRenderPass::ManaRenderPass(std::shared_ptr<Internal::VulkanRenderPass> render_pass, ManaInstance *owner)
{
    this->vulkan_render_pass = render_pass;
    this->owner = owner;
}

ManaVK::ManaRenderPass::~ManaRenderPass() {
    release();
}

void ManaVK::ManaRenderPass::release() {
    if (owner == nullptr) {
        throw std::runtime_error("Owner was nullptr! This object is unable to be released, causing a memory leak!");
    }

    if (vulkan_render_pass != nullptr) {
        auto func = [vulkan_render_pass = vulkan_render_pass](ManaInstance* p_instance) {
            vulkan_render_pass->release(p_instance->get_vulkan_instance()->get_vk_device());
        };

        owner->enqueue_release(func);
        vulkan_render_pass = nullptr;
    }
}


void ManaRenderPass::begin(ManaRenderContext& context) {
    if (vulkan_render_pass == nullptr) {
        throw std::runtime_error("vulkan_render_pass was nullptr!");
    }

    Internal::VulkanRenderPass::StateInfo info {};
    {
        info.vulkan_render_target = context.get_vulkan_rt();

        // TODO: Render rect
        // TODO: Clear values
        info.vk_clear_values.push_back({});
        info.vk_clear_values.push_back({});
    }

    vulkan_render_pass->begin(context.get_owner()->get_vulkan_instance().get(), info);
}

void ManaRenderPass::end(ManaVK::ManaRenderContext &context) {
    if (vulkan_render_pass == nullptr) {
        throw std::runtime_error("vulkan_render_pass was nullptr!");
    }

    Internal::VulkanRenderPass::StateInfo info {};
    {
        info.vulkan_render_target = context.get_vulkan_rt();
    }

    vulkan_render_pass->end(info);
}