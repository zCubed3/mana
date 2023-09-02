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

#include "vulkan_window.hpp"

#include <vulkan/vulkan.h>

#include <SDL.h>
#include <SDL_vulkan.h>

#include <stdexcept>

using namespace ManaVK;

// TODO: Window position?
Internal::VulkanWindow::VulkanWindow(const std::string &name, int width, int height) {
    handle = SDL_CreateWindow(
        name.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_VULKAN
    );
}

void Internal::VulkanWindow::create_surface(VkInstance vk_instance) {
    if (vk_instance == nullptr) {
        throw std::runtime_error("vk_instance was nullptr!");
    }

    if (!SDL_Vulkan_CreateSurface(handle, vk_instance, &vk_surface)) {
        throw std::runtime_error("SDL_Vulkan_CreateSurface failed!");
    }
}
