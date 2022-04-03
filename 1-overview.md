### Overview

Vulkan is designed to offer more flexibility to developers.

Where previous graphic APIs were based on GPU with fixed function Vulkan can leverage modern and
mature GPU functionality to offer more control.

In addition Vulkan works well in a multithreaded environment which accomodates current trend.
Vulkan offers:
  - more verbose API in order to better express intent,
  - allow multiple thread to create and submit commands in parallel,
  - switch to a standardized bytecode with only one compiler for the shading language and
  - unify the graphic and compute API.

Big picture on how to render a triangle. You need to:
  1. **Instance and physical devices**: Create a `VkInstance`. Here wou will specify the extension
     you want to use and select a `VkPhysicalDevice` by optionaly query the capability you will
     need.
  2. **Logical devices and queues**: Create a `VkDevice` (logical device) where the feature you will
     need will be selected through `VkPhysicalDeviceFeatures` (multi viewport, 64 bits floats, etc)
     and specifying which `VkQueue` you will need. Different queues will be used for graphic,
     compute or memory commands.  Commands are inserted asynchronously in the queues.
  3. **Window surface and swap chains**: In order to display the rendered image you will need to
     create a surface (`VkSurfaceKHR`) and a swap chain (`VkSwapchainKHR`). The `KHR` suffix means
     these are part of the _Window System Interface_ extension. Swap chain can currently be double-
     or triple-buffering. On some platfform, the use of `VK_KHR_display` and
     `VK_KHR_display_platform` extensions can be used for fullscreen rendering.
  4. **Image views and framebuffers**: `VkImage` will represent a specific part of an image to be
     used and `VkFrameBuffer` assembles images as color, depth and stencil targets. TODO: need to
     dig more into this part.
  5. **Render pass**: This is where we describe the clear color and the image to display our
     triangle.
  6. **Graphic pipeline**: The pipeline defines the 'configuratble state of the graphic cards' like
     the viewport size of depth buffer operation (TODO: understand what that means). This is where
     GPU programs are, created as `VkShaderModule` from shader byte code, are described. A pipeline
     is created statically and very few parameters can be dyncamically changed (except viewport size
     or clear color) and there are no default values. Pipeline must be created in advanced and
     switch between if needed.
  7. **Command pools and command buffers**: A `VkCommandPool` is used to create `VkCommandBuffer`
     which is used to record commands (e.g. drawing operations) and then submit them to a queue. For
     example, to draw a triangle:
     1. begin the render pass,
     2. bind the graphic pipeline
     3. draw 3 vertices
     4. end the render pass
   8. **Main loop**: We will acquire an image from the swap chain (`vkAcquireImageKHR`), then render
      in that image by submitting commands to a particular queue (`vkQueueSubmit`) and then
      presenting the image again to the swap chain (`vkQueuePresentKHR`). Queues are asynchronous so
      semaphores are used to synchronize with the swap chain calls.

### Coding conventions

  - function starts with `vk`.
  - Enumeration and struct starts with `Vk`.
  - Macro starts with `VK_`.
  - Struct are used to provide info to functions:

```c
VkXXXCreateInfo createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_XXX_CREATE_INFO;
createInfo.pNext = nullptr;
createInfo.foo = ...;
createInfo.bar = ...;

VkXXX object;
if (vkCreateXXX(&createInfo, nullptr, &object) != VK_SUCCESS) {
  std::cerr << "failed to create object" << std::endl;
  return false;
}
```
Struct will always have a `sType` field containing the type of the structure (:/). the `pNext` will
point to the next element in the list of structures which can be extensions for example.

Functions allocating struct will always have a `VkAllocationCallbacks` to provide a custom
allocator.

Functions returns a `VkResult` which is either `VK_Success` or an error code.

### Validation layers

By default, Vulkan run in performance mode with no debugging facility and limited error checking.
Code can crash or worst, work on some hardware but not other when we do something wrong.

Validation layers are used for debugging and checking.

