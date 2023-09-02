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

#ifndef MANA_VULKAN_SAMPLER_HPP
#define MANA_VULKAN_SAMPLER_HPP

#include <vulkan/vulkan.h>

#include <optional>

namespace ManaVK::Internal {
    class VulkanSampler {
    public:
        struct SamplingSettings {
            VkFilter vk_filter_mag = VK_FILTER_LINEAR;
            VkFilter vk_filter_min = VK_FILTER_LINEAR;

            VkSamplerAddressMode vk_address_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            VkSamplerAddressMode vk_address_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            VkSamplerAddressMode vk_address_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;

            std::optional<float> anisotropic_samples;

            // The maximum mip LOD clamp, when empty, we don't clamp!
            std::optional<float> mip_clamp;
            float mip_min = 0;
            float mip_bias = 0;

            VkSamplerMipmapMode vk_mip_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            std::optional<VkCompareOp> vk_compare_op;
        };

    protected:
        VkSampler vk_sampler = nullptr;

    public:
        VulkanSampler(VkDevice vk_device, const SamplingSettings& settings);
    };
}

#endif//MANA_VULKAN_SAMPLER_HPP
