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

#include "mana_enums.hpp"

#include <vulkan/vulkan.h>

int ManaVK::mana_color_format_to_vk_format(ManaVK::ManaColorFormat format) {
    switch (format) {
        default:
            return VK_FORMAT_UNDEFINED;

        case ManaColorFormat::B8G8R8A8_UNorm:
            return VK_FORMAT_B8G8R8A8_UNORM;

        case ManaColorFormat::R8G8B8A8_UNorm:
            return VK_FORMAT_R8G8B8A8_UNORM;
    }
}

int ManaVK::mana_depth_format_to_vk_format(ManaVK::ManaDepthFormat format) {
    switch (format) {
        default:
            return VK_FORMAT_UNDEFINED;

        case ManaDepthFormat::D16_UNorm_S8_UInt:
            return VK_FORMAT_D16_UNORM_S8_UINT;

        case ManaDepthFormat::D24_UNorm_S8_UInt:
            return VK_FORMAT_D24_UNORM_S8_UINT;

        case ManaDepthFormat::D32_SFloat_S8_UInt:
            return VK_FORMAT_D32_SFLOAT_S8_UINT;
    }
}

int ManaVK::mana_samples_to_vk_samples(ManaVK::ManaSamples samples) {
    switch (samples) {
        default:
            return VK_SAMPLE_COUNT_1_BIT;

        case ManaSamples::Two:
            return VK_SAMPLE_COUNT_2_BIT;

        case ManaSamples::Four:
            return VK_SAMPLE_COUNT_4_BIT;

        case ManaSamples::Eight:
            return VK_SAMPLE_COUNT_8_BIT;
    }
}