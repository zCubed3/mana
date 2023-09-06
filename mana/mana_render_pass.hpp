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

#ifndef MANA_MANA_RENDER_PASS_HPP
#define MANA_MANA_RENDER_PASS_HPP

#include <memory>

#include <mana/mana_render_context.hpp>

namespace ManaVK::Internal {
    class VulkanRenderPass;
}

namespace ManaVK {
    class ManaInstance;

    class ManaRenderPass {
    protected:
        ManaInstance *owner;
        std::shared_ptr<Internal::VulkanRenderPass> vulkan_render_pass;

    public:
        ManaRenderPass(std::shared_ptr<Internal::VulkanRenderPass> render_pass, ManaInstance *owner);
        ~ManaRenderPass();

        void begin(ManaRenderContext& context);
        void end(ManaRenderContext& context);

        void release();

    public:
        //
        // Getters
        //
        [[nodiscard]]
        std::shared_ptr<Internal::VulkanRenderPass> get_vulkan_render_pass() const {
            return vulkan_render_pass;
        }
    };
}

#endif//MANA_MANA_RENDER_PASS_HPP
