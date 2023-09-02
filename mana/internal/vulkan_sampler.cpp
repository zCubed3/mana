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

#include "vulkan_sampler.hpp"

#include <vulkan/vk_enum_string_helper.h>

#include <stdexcept>
#include <iostream>

#define LOG_INLINE(args) std::cout << "[ManaVK::Internal::VulkanSampler]: "<< args
#define LOG(args) LOG_INLINE(args) << std::endl

using namespace ManaVK;

Internal::VulkanSampler::VulkanSampler(VkDevice vk_device, const SamplingSettings &settings) {
    if (vk_device == nullptr) {
        throw std::runtime_error("vk_device was nullptr!");
    }

    VkSamplerCreateInfo sampler_create_info{};
    {
        sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_create_info.magFilter = settings.vk_filter_mag;
        sampler_create_info.minFilter = settings.vk_filter_min;

        sampler_create_info.addressModeU = settings.vk_address_u;
        sampler_create_info.addressModeV = settings.vk_address_v;
        sampler_create_info.addressModeW = settings.vk_address_w;

        sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_create_info.unnormalizedCoordinates = VK_FALSE;
        sampler_create_info.compareEnable = settings.vk_compare_op.has_value();
        sampler_create_info.compareOp = settings.vk_compare_op.value_or(VK_COMPARE_OP_ALWAYS);

        // It's up to the user to verify if aniso is supported
        sampler_create_info.anisotropyEnable = settings.anisotropic_samples.has_value();
        sampler_create_info.maxAnisotropy = settings.anisotropic_samples.value_or(0.0F);

        sampler_create_info.mipmapMode = settings.vk_mip_mode;
        sampler_create_info.mipLodBias = settings.mip_bias;
        sampler_create_info.minLod = settings.mip_min;
        sampler_create_info.maxLod = settings.mip_clamp.value_or(VK_LOD_CLAMP_NONE);
    }

    VkResult result = vkCreateSampler(vk_device, &sampler_create_info, nullptr, &vk_sampler);

    if (result != VK_SUCCESS) {
        LOG("vkCreateSampler failed with error code (" << string_VkResult(result) << ")");
        throw std::runtime_error("vkCreateSampler failed! Please check the log above for more info!");
    }
}
