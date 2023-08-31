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

#ifndef SAPPHIRE_MANA_MEMORY_HPP
#define SAPPHIRE_MANA_MEMORY_HPP

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <memory>
#include <vector>

namespace Sapphire::Mana {
    class ManaProvider;

    // A block of memory allocated by the provider
    class MemoryBlock {
        friend class StagingMemoryPool;

    protected:
        // TODO: Rather than storing the buffer, instead require the user pass it back into the provider?
        VkBuffer vk_parent_buffer = nullptr;
        VmaVirtualAllocation vma_valloc;
        VkDeviceSize vk_offset;
        size_t chunk_index = -1;
        bool upload_complete = true;

    public:
        MemoryBlock() = delete;
        MemoryBlock(size_t chunk_index, VkBuffer vk_parent_buffer, VmaVirtualAllocation vma_valloc, VkDeviceSize vk_offset);

        VkBuffer get_vk_buffer();
        VkDeviceSize get_vk_offset();
        size_t get_chunk_index();

        [[nodiscard]]
        bool is_uploaded() const {
            return upload_complete;
        }
    };

    class MemoryPoolChunk {
    public:
        VmaVirtualBlock vma_vblock;
        VkBuffer vk_buffer;
        VmaAllocation vma_alloc;
        VmaAllocationInfo vma_alloc_info;
        size_t chunk_index = -1;

        std::shared_ptr<MemoryBlock> try_alloc(size_t size);
    };

    // TODO: Track our allocations intelligently?
    // TODO: Actually make this a pool
    class MemoryPool {
    protected:
        // TODO: Actually multiple v-allocs if we go over the buffer size!!!
        std::vector<MemoryPoolChunk> chunks;
        size_t size;
        uint32_t usage;
        uint32_t flags;

        virtual void push_chunk(VulkanProvider *p_provider);

    public:
        MemoryPool() = delete;
        MemoryPool(VulkanProvider *p_provider, size_t size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags);

        std::shared_ptr<MemoryBlock> alloc(size_t size);
    };

    // The same as a MemoryPool except that we transfer the copy data over instantly into our mapped buffer handles
    class StagingMemoryPool : public MemoryPool {
    protected:
        struct StagedUpload {
            std::shared_ptr<MemoryBlock> dst_block;
            std::shared_ptr<MemoryBlock> src_block;
            size_t size;
        };

        std::vector<char*> handles {};
        std::vector<StagedUpload> upload_queue;

        void push_chunk(Sapphire::Graphics::VulkanProvider *p_provider) override;
        void validate_handles(VulkanProvider *p_provider);

    public:
        StagingMemoryPool() = delete;
        StagingMemoryPool(VulkanProvider *p_provider, size_t size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags);

        // Flushes the upload queue, uploads everything synchronously
        // TODO: Async flushing?
        void flush(VulkanProvider *p_provider);

        // TODO: Async????????
        void enqueue_upload(size_t size, void* src, std::shared_ptr<Graphics::MemoryBlock> dst);
    };
}

#endif//SAPPHIRE_MEMORY_BLOCK_HPP
