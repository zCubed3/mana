cmake_minimum_required(VERSION 3.15)
project(Mana)

set(CMAKE_CXX_STANDARD 17)

#
# Dependencies
#
find_package(Vulkan REQUIRED)
add_subdirectory(third_party/VulkanMemoryAllocator)

# TODO: Include our own copy of SDL2?

#
# Library
#
set(MANA_SOURCES
    "mana/internal/vulkan_instance.cpp"
    "mana/internal/vulkan_queue.cpp"
    "mana/internal/vulkan_window.cpp"
    "mana/internal/vulkan_sampler.cpp"
    "mana/internal/vulkan_image.cpp"
    "mana/internal/vulkan_render_pass_builder.cpp"
    "mana/internal/vulkan_render_pass.cpp"

    "mana/builders/mana_render_pass_builder.cpp"

    "mana/mana_instance.cpp"
    "mana/mana_window.cpp"
    "mana/mana_enums.cpp"
    "mana/mana_image.cpp"
)

add_library(Mana STATIC ${MANA_SOURCES})

target_include_directories(Mana PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} SDL2)

target_link_libraries(Mana PUBLIC ${Vulkan_LIBRARY} VulkanMemoryAllocator SDL2)

# TODO: Allow the user to optionally include Vulkan headers?
target_include_directories(Mana PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    VulkanMemoryAllocator
)