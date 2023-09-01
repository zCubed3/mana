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

#ifndef MANA_MANA_INSTANCE_HPP
#define MANA_MANA_INSTANCE_HPP

#include <memory>
#include <string>
#include <vector>

namespace ManaVK::Internal {
    class VulkanInstance;
}

namespace ManaVK {
    // Mana is generic, but tailored towards game usage
    // This means a transfer, graphics, and present queue are all allocated by default
    // Mana will handle creating windows for you
    // Initializing the instance provides you with the main window used for Vulkan creation
    class ManaInstance {
    public:
        struct ManaFeatures {
            // By default, basic raster is REQUIRED!

            // TODO: Implement raytracing
            bool raytracing = false;
        };

        struct ManaDebugging {
            bool enable_khronos_layer = false;
            bool verbose = false;
        };

        struct ManaVersion {
            uint32_t major = 1;
            uint32_t minor = 0;
            uint32_t patch = 0;
            uint32_t rev = 0;

            ManaVersion(uint32_t major, uint32_t minor, uint32_t patch, uint32_t rev) {
                this->major = major;
                this->minor = minor;
                this->patch = patch;
                this->rev = rev;
            }
        };

        struct ManaWindowSettings {
            std::string title = "ManaVK Window";
            int width = 1280;
            int height = 720;
        };

        struct ManaConfig {
            ManaFeatures features;
            ManaDebugging debugging;
            ManaWindowSettings window_settings;

            std::string engine_name = "ManaVK Engine";
            ManaVersion engine_version = ManaVersion(1, 0, 0, 0);

            std::string app_name = "ManaVK Application";
            ManaVersion app_version = ManaVersion(1, 0, 0, 0);
        };

    protected:
        Internal::VulkanInstance *vulkan_instance = nullptr;

    public:
        ManaInstance(const ManaConfig& config);

    protected:
        //
        // Helpers
        //
        uint32_t get_api_version(const ManaFeatures& features) const;
    };
}

#endif//MANA_MANA_INSTANCE_HPP
