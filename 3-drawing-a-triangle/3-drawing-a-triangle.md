# Setup

## Base code

Vulkan can throw exception for example when an extension is missing. They are
caught with `std::exception` from `<stdexcept>`.

```C++
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
```

By using the `GLFW_INCLUDE_VULKAN`, we automatically include `#include <vulkan/vulkan.h>`.

Many object will be created and allocated. All those object must be destroyed of freed after use.
The best strategy is to use C++ proxy that you will implement and will handle that using RAII.
For this tutorial, we will be dealing with allocating/freeing directly.

## Instance

Nothing.

## Extensions

Many vulkan features comes from extension, especially the ones that will depends on the hardware or
the operating systems. Some of those extensions are global, for example the extensions which will
allow to display an image on screen is global. Some other extensions are device dependent and will
be present or not depending on the capability of your GPU. Some GPU do not have Swap Chains for
example because they are not meant to display image on screen but only perfom calculations.

Physical devices holds the list of available extension for each device but you enable those
extension when you create the logical device.

## Validation layers

By default, no checking or validation is done on parameters passed to Vulkan functions. Undefined
behavior is common. (passing the wrong enums, using a feature without requesting it when creating
the logical device).

There used to be two type of layers: instance layers and device layers. The latter are deprecated
and instance layers are now intercepting all calls (global vulkan objects and and calls related to
a specific GPU).

They are multiple validation layer available, For example:
-**`VK_LAYER_MESA_device_select`**: ??
-**`VK_LAYER_KHRONOS_validation`**: A bundle validation (include multiple validation layers) which
 enable validation of various API.
-**`VK_LAYER_MESA_overlay`**: ??

Extensions can also be used for debugging. For example `VK_EXT_debug_utils` is used to plug a debug
callback which will be called on on every Vulkan API call.

All the details on layers
[here](https://vulkan.lunarg.com/doc/sdk/latest/linux/layer_configuration.html).

## Physical Devices

Physical devices can be enumerated just like layers and extensions and many other Vulkan objects.

You can query properties like name, type and supported Vulkan version with
`vkGetPhysicalDeviceProperties` or their available features like texture compression or 64 bits floats
with `vkGetPhysicalDeviceFeatures`.

## Queues

Nothing.

## Logical Devices

Although we check for the queue capability when we have created a physical device, we create the
actual queues when we create the logical device.

# Presentation

## Window Surface

Windows surface (`VkSurfaceKHR`) are used to present images to the screen and are **optional**. They
come from the WSI (Window System Integration) extensions. They are automatically included through
the call to `glfwGetRequiredInstanceExtensions`.

There is a couple of gotcha:
  1. When destroying the surface make sure to do it before you destroy the instance.
  2. All devices/queues are not compatible with surfaces and this must be check when selecting a
     queue.

In addition of creating a graphic queue, you also need to create a presentation queue. In practice
they probably will be the same but it's a little effort to separate them and thus covering all your
bases.

## Swap chains

The swap chain is essentially a queue of images that are waiting to be presented to the screen.
Our application will acquire such an image to draw to it, and then return it to the queue.

Like Window Surfaces, Swap Chains are coming from an extension (`VK_KHR_swapchain`)
as they depend on the underlying OS and hardware.
