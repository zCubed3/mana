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

#include "vulkan_image.hpp"

#include <mana/internal/vulkan_instance.hpp>

#include <vulkan/vk_enum_string_helper.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <iostream>

#define LOG_INLINE(args) std::cout << "[ManaVK::Internal::VulkanImage]: "<< args
#define LOG(args) LOG_INLINE(args) << std::endl

using namespace ManaVK::Internal;

VulkanImage::VulkanImage(VulkanInstance *vulkan_instance, const ImageSettings &settings) {
    if (vulkan_instance == nullptr) {
        throw std::runtime_error("vulkan_instance was nullptr!");
    }

    //
    // Enum translation
    //
    uint32_t layer_count = settings.array_size;
    VkImageType vk_image_type = VK_IMAGE_TYPE_2D;
    VkImageCreateFlags vk_image_flags = 0;
    VkImageViewType vk_view_type = VK_IMAGE_VIEW_TYPE_2D;
    switch (settings.shape) {
        case ImageShape::Shape1D:
            if (settings.array_size > 1) {
                vk_view_type = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            }

            vk_image_type = VK_IMAGE_TYPE_1D;
            break;

        case ImageShape::Shape2D:
            if (settings.array_size > 1) {
                vk_view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            }

            break;

        case ImageShape::Shape3D:
            vk_image_type = VK_IMAGE_TYPE_3D;
            vk_view_type = VK_IMAGE_VIEW_TYPE_3D;
            break;

        case ImageShape::ShapeCube:
            vk_image_type = VK_IMAGE_TYPE_2D;
            vk_image_flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            vk_view_type = VK_IMAGE_VIEW_TYPE_CUBE;
            break;
    }

    //
    // Mip determining
    //
    uint32_t mip_levels = 1;

    if (settings.generate_mipmaps) {
        // Taken from the Vulkan Tutorial: https://vulkan-tutorial.com/Generating_Mipmaps
        mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(settings.vk_extent.width, settings.vk_extent.height)))) + 1;
    }


    //
    // Image creation
    //
    VkImageCreateInfo image_info {};
    {
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

        image_info.extent.width = settings.vk_extent.width;
        image_info.extent.height = settings.vk_extent.height;
        image_info.extent.depth = settings.vk_extent.depth;

        image_info.arrayLayers = layer_count;
        image_info.mipLevels = mip_levels;

        image_info.samples = settings.vk_samples;
        image_info.format = settings.vk_format;

        image_info.imageType = vk_image_type;
        image_info.flags = vk_image_flags;

        image_info.tiling = settings.vk_tiling;
        image_info.initialLayout = settings.vk_layout;

        image_info.usage = settings.vk_usage_flags;
        image_info.sharingMode = settings.vk_sharing_mode;
    }

    VkResult result = vkCreateImage(vulkan_instance->get_vk_device(), &image_info, nullptr, &vk_image);
    if (result != VK_SUCCESS) {
        LOG("vkCreateImage failed with error code (" << string_VkResult(result) << ")");
        throw std::runtime_error("vkCreateImage failed! Please check the log above for more info!");
    }

    //
    // Memory allocation
    //
    {
        VkMemoryRequirements mem_requirements{};
        vkGetImageMemoryRequirements(vulkan_instance->get_vk_device(), vk_image, &mem_requirements);

        VmaAllocationCreateInfo vma_alloc_info{};
        VmaAllocationInfo vma_allocation_info;

        vmaAllocateMemoryForImage(vulkan_instance->get_vma_allocator(), vk_image, &vma_alloc_info, &vma_allocation, &vma_allocation_info);

        vmaBindImageMemory(vulkan_instance->get_vma_allocator(), vma_allocation, vk_image);
    }

    //
    // View creation
    //
    VkImageViewCreateInfo view_info {};
    {
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

        view_info.image = vk_image;
        view_info.viewType = vk_view_type;

        view_info.format = settings.vk_format;
        view_info.components = settings.vk_components;

        view_info.subresourceRange.aspectMask = settings.vk_aspect_flags;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = mip_levels;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = layer_count;
    }

    result = vkCreateImageView(vulkan_instance->get_vk_device(), &view_info, nullptr, &vk_view);
    if (result != VK_SUCCESS) {
        LOG("vkCreateImageView failed with error code (" << string_VkResult(result) << ")");
        throw std::runtime_error("vkCreateImageView failed! Please check the log above for more info!");
    }
}

void VulkanImage::release(VulkanInstance *vulkan_instance) {
    if (vulkan_instance == nullptr) {
        throw std::runtime_error("vulkan_instance was nullptr! Can't release Vulkan resources, this is a memory leak!");
    }

    if (vk_image != nullptr) {
        vkDestroyImage(vulkan_instance->get_vk_device(), vk_image, nullptr);
        vk_image = nullptr;
    }

    if (vk_view != nullptr) {
        vkDestroyImageView(vulkan_instance->get_vk_device(), vk_view, nullptr);
        vk_view = nullptr;
    }

    if (vma_allocation != nullptr) {
        vmaFreeMemory(vulkan_instance->get_vma_allocator(), vma_allocation);
        vma_allocation = nullptr;
    }
}
