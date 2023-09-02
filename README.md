# ManaVK - [Sapphire Engine's Vulkan Abstraction](https://github.com/zCubed3/sapphire)

## Dependencies
* [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)

## LLAPI (Low Level API) - `ManaVK::Internal`
* A thin layer over Vulkan, reducing the amount of things you can do, but for implementation reasons
* Still allows the user to directly touch Vulkan structures, but is unsafe
* Helpers exist, but purely for avoiding code repetition and HLAPI QoL

## HLAPI (High Level API) - `ManaVK`
* Makes Vulkan more 'stateful', almost like OpenGL
* Helps reduce implementation complexity by preventing direct Vulkan access (this is why the `ManaVK::Internal` namespace exists!)
* Provides the user with methods to easily create a Vulkan instance and get it running