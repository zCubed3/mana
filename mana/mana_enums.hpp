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

#ifndef MANA_MANA_ENUMS_HPP
#define MANA_MANA_ENUMS_HPP

namespace ManaVK {
    // Indicates the amount of multisampling (MSAA)
    enum class ManaSamples {
        One,
        Two,
        Four,
        Eight
    };

    // Defines the type of rendering we're intending on using
    // Be it raster, compute, or raytracing
    enum class ManaRenderType {
        Raster, Compute, Raytracing
    };

    // ManaFormats are a subset of VkFormats
    // The purpose is to hide many options people wouldn't use for a game engine
    // Plus you can more easily validate if a mana format is supported
    enum class ManaColorFormat {
        B8G8R8A8_UNorm,
        R8G8B8A8_UNorm
    };

    enum class ManaDepthFormat {
        D16_UNorm_S8_UInt,
        D24_UNorm_S8_UInt,
        D32_SFloat_S8_UInt
    };

    enum class ManaColorSpace {
        NonLinear_sRGB

        // TODO: HDR
    };

    int mana_color_format_to_vk_format(ManaColorFormat format);
    int mana_depth_format_to_vk_format(ManaDepthFormat format);
    int mana_samples_to_vk_samples(ManaSamples samples);
}

#endif//MANA_MANA_ENUMS_HPP
