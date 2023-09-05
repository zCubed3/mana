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

#ifndef MANA_VULKAN_IMAGE_HPP
#define MANA_VULKAN_IMAGE_HPP

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace ManaVK::Internal {
    class VulkanInstance;

    class VulkanImage {
    public:
        enum class ImageShape {
            Shape1D,
            Shape2D,
            Shape3D,
            ShapeCube
        };

        struct ImageSettings {
            VkFormat vk_format;
            VkExtent3D vk_extent {};

            VkSampleCountFlagBits vk_samples = VK_SAMPLE_COUNT_1_BIT;
            VkImageUsageFlags vk_usage_flags = 0;
            VkImageAspectFlags vk_aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
            VkImageTiling vk_tiling = VK_IMAGE_TILING_OPTIMAL;
            VkImageLayout vk_layout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkSharingMode vk_sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
            VkComponentMapping vk_components {};

            ImageShape shape = ImageShape::Shape2D;
            uint32_t array_size = 1;

            bool generate_mipmaps = true;
        };

    protected:
        VkImage vk_image = nullptr;
        VkImageView vk_view = nullptr;
        VmaAllocation vma_allocation = nullptr;

    public:
        VulkanImage(VulkanInstance *vulkan_instance, const ImageSettings& settings);

        void release(VulkanInstance *vulkan_instance);

        //
        // Getters
        //
        [[nodiscard]]
        VkImage get_vk_image() const {
            return vk_image;
        }

        [[nodiscard]]
        VkImageView get_vk_view() const {
            return vk_view;
        }
    };
}

#endif//MANA_VULKAN_IMAGE_HPP
