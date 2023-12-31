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

#include "mana_window.hpp"

#include <mana/internal/vulkan_window.hpp>

#include <mana/mana_instance.hpp>
#include <mana/mana_render_context.hpp>

#include <stdexcept>

using namespace ManaVK;

ManaWindow::ManaWindow(Internal::VulkanWindow *vulkan_window, ManaInstance *owner) {
    this->vulkan_window = vulkan_window;
    this->owner = owner;
}

ManaRenderContext ManaWindow::new_frame() {
    // TODO: Frames in flight
    vulkan_window->await_frame(owner->get_vulkan_instance().get());
    return ManaRenderContext(vulkan_window, owner);
}

void ManaWindow::flush(ManaInstance *mana_instance) {
    if (dirty) {
        vulkan_window->recreate_swapchain(mana_instance->get_vulkan_instance().get());
        dirty = false;
    }
}
