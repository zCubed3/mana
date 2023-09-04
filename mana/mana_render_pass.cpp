#include "mana_render_pass.hpp"

#include <mana/internal/vulkan_render_pass.hpp>
#include <mana/internal/vulkan_instance.hpp>

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
    }
}