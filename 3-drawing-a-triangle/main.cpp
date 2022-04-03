// ls main.cpp | entr -rc bash -c 'make && ./VulkanTest'
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string.h>
// For InstanceExtensionRequested
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// GLFW_INCLUDE_VULKAN Will include the vulkin header
// #include <vulkan/vulkan.h>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
  "VK_LAYER_KHRONOS_validation",
};

#ifdef DEBUG
  const bool enableValidationLayers = true;
#else
  const bool enableValidationLayers = false;
#endif

typedef std::vector<const char *> ExtensionInfo;

/**
 * A debug callback which can be used when the extension VK_EXT_DEBUG_UTILS_EXTENSION_NAME is
 * enabled.
 * @param  messageSeverity A bit field to be matched against the following macro:
 *                         `VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT`: Diagnostic message
 *                         `VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT`: Informational message
 *                         like the creation of a resource.
 *                         `VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT`: Message about behavior
 *                         that is not necessarily an error, but very likely a bug in your
 *                         application.
 *                         `VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT`: Message about behavior
 *                         that is invalid and may.
 * @param  messageType     Type of message to matched against the following macro:
 *                         `VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT`: Some event has happened
 *                         that is unrelated to the specification or performance.
 *                         `VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT`: Something has happened
 *                         that violates the specification or indicates a possible mistake.
 *                         `VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT`: Potential non-optimal
 *                         use of Vulkan.
 * @param  pCallbackData   struct containing the details of the message itself, with the most
 *                         important members being:
 *                         `pMessage`: The debug message as a null-terminated string
 *                         `pObjects`: Array of Vulkan object handles related to the message
 *                         `objectCount`: Number of objects in array
 * @param  pUserData       User data provided when setting up the debug call back. Can be logger for
 *                         example.
 * @return                 ??
 */
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

/**
 * Upon instantiation, will query the list of extension available on your system and store the
 * result in public variables.
 */
class InstanceExtensionEnumerator {
public:
  InstanceExtensionEnumerator() {
    vkEnumerateInstanceExtensionProperties(nullptr, &this->extensionCount, nullptr);
    extensions.resize(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
  }

  uint32_t extensionCount = 0;
  std::vector<VkExtensionProperties> extensions;
};

/**
 * Upon instantiation, will query all the validation layers available on your system and store the
 * result in public variables.
 */
class ValidationLayerEnumerator {
public:
  ValidationLayerEnumerator() {
    vkEnumerateInstanceLayerProperties(&this->layerCount, nullptr);
    this->layers.resize(this->layerCount);
    vkEnumerateInstanceLayerProperties(&this->layerCount, this->layers.data());
  }

  uint32_t layerCount = 0;
  std::vector<VkLayerProperties> layers;
};

/**
 * List the queue families available on the physical device.
 */
class QueueFamilyEnumerator {
public:
  QueueFamilyEnumerator(VkPhysicalDevice device) {
    vkGetPhysicalDeviceQueueFamilyProperties(device, &this->queueCount, nullptr);
    this->queues.resize(this->queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &this->queueCount, this->queues.data());
  }

  uint32_t queueCount = 0;
  std::vector<VkQueueFamilyProperties> queues;
};

/**
 * Aggregate all the useful information for a physical device.
 */
struct PhysicalDevice {
  VkPhysicalDevice device;
  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;
  // Those indices will be used at the creation of the logical device.
  std::map<VkQueueFlagBits, uint32_t> queueFamilyIndices;
  int32_t presentationStateQueueIndex;
  std::vector<VkExtensionProperties> availableExtensions;
};

/**
 * List the physical devices on the provided instance with their properties, features and queue
 * family indices.
 */
class PhysicalDeviceEnumerator {
public:
  PhysicalDeviceEnumerator(const VkInstance instance, const VkSurfaceKHR surface) {
    vkEnumeratePhysicalDevices(instance, &this->physicalDeviceCount, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(this->physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance, &this->physicalDeviceCount, physicalDevices.data());

    for (auto device: physicalDevices) {
      VkPhysicalDeviceProperties deviceProperties;
      vkGetPhysicalDeviceProperties(device, &deviceProperties);

      VkPhysicalDeviceFeatures deviceFeatures;
      vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

      std::map<VkQueueFlagBits, uint32_t> queueFamilyIndices = getQueueIndices(device);
      int32_t presentationStateQueueIndex = -1;
      findQueueWithPresentationCapability(device, surface, queueFamilyIndices,
        &presentationStateQueueIndex);

      uint32_t extensionCount;
      vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
      std::vector<VkExtensionProperties> availableExtensions(extensionCount);
      vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
        availableExtensions.data());

      this->physicalDevices.push_back({
        .device = device,
        .properties = deviceProperties,
        .features = deviceFeatures,
        .queueFamilyIndices = queueFamilyIndices,
        .presentationStateQueueIndex = presentationStateQueueIndex,
        .availableExtensions = availableExtensions,
      });
    }
  }

  /**
   * Creates a map containing the indices of the queue family on the provided device.
   */
  std::map<VkQueueFlagBits, uint32_t> getQueueIndices(VkPhysicalDevice device) {
    QueueFamilyEnumerator queueFamilyEnumerator(device);
    std::map<VkQueueFlagBits, uint32_t> queueFamilyIndices;

    for (size_t idx = 0; auto &queue: queueFamilyEnumerator.queues) {
      if (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        queueFamilyIndices[VK_QUEUE_GRAPHICS_BIT] = idx;
      }
      if (queue.queueFlags & VK_QUEUE_COMPUTE_BIT) {
        queueFamilyIndices[VK_QUEUE_COMPUTE_BIT] = idx;
      }
      if (queue.queueFlags & VK_QUEUE_TRANSFER_BIT) {
        queueFamilyIndices[VK_QUEUE_TRANSFER_BIT] = idx;
      }
      if (queue.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
        queueFamilyIndices[VK_QUEUE_SPARSE_BINDING_BIT] = idx;
      }
      if (queue.queueFlags & VK_QUEUE_PROTECTED_BIT) {
        queueFamilyIndices[VK_QUEUE_PROTECTED_BIT] = idx;
      }
    }

    return queueFamilyIndices;
  }

  // Find a queue family that supports presentation state. Set the index of the family in queueIndex
  // and return true if found. Return false otherwise.
  uint32_t findQueueWithPresentationCapability(VkPhysicalDevice device, VkSurfaceKHR surface,
    std::map<VkQueueFlagBits, uint32_t> queueFamilyIndices, int32_t *queueIndex) {
    for (auto const& familyIndices : queueFamilyIndices) {
      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, familyIndices.first, surface, &presentSupport);
      if (presentSupport) {
        *queueIndex = familyIndices.first;
        return 0;
      }
    }

    return 1;
  }

  uint32_t physicalDeviceCount = 0;
  std::vector<PhysicalDevice> physicalDevices;
};

class HelloTriangleApplication {
public:
  void run() {
#ifdef DEBUG
    this->printVulkanBanner();
#endif
    this->initWindow();
    this->initVulkan();
    this->mainLoop();
    this->cleanup();
  }

private:
  void initWindow() {
    // Initialize the Window library.
    glfwInit();
    // This function expects an OpenGL context. But in our case, we are passing GLFW_NO_API.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // Because handling resized windows takes special care that we'll look into later, disable it
    // for now with another window hint call:
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    this->window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
  }

  void initVulkan() {
    // Create the VkInstance
    this->instance = this->createInstance();
    this->setupDebugMessenger();
    this->surface = this->createSurface(this->instance, this->window);
    this->physicalDevice = this->getPhysicalDevice(this->instance, this->surface);
    this->device = this->getLogicalDevice(this->physicalDevice);
    // Retrieve the graphic queue
    vkGetDeviceQueue(this->device, this->physicalDevice.queueFamilyIndices[VK_QUEUE_GRAPHICS_BIT],
      0, &this->graphicsQueue);
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(this->window)) {
      glfwPollEvents();
    }
  }

  void cleanup() {
    this->unSetupDebugMessenger();
    // Goes with vkCreateDevice
    vkDestroyDevice(this->device, nullptr);
    // Goes with glfwCreateWindowSurface.
    // Surface must be destroyed before the instance.
    vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
    // Goes with vkCreateInstance
    vkDestroyInstance(this->instance, nullptr);
    // For the glfw library
    glfwDestroyWindow(this->window);
    glfwTerminate();
  }

private:
  VkInstance createInstance() {
    if (enableValidationLayers && !this->checkValidationLayerSupport(validationLayers)) {
      throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = nullptr,
      .pApplicationName = "Hello Triangle",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "No Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_0,
    };

    // Vulkan is a platform agnostic API, which means that you need an extension to interface with
    // the window system. GLFW has a handy built-in function that returns the extension(s) it needs
    // to do that
    ExtensionInfo glfwExtensions = this->glfwExtensionInfo();
    ExtensionInfo debugExtensions = this->getDebugExtensions();
    ExtensionInfo extensionInfo;
    std::merge(
      glfwExtensions.begin(), glfwExtensions.end(),
      debugExtensions.begin(), debugExtensions.end(),
      std::back_inserter(extensionInfo));

    VkInstanceCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &appInfo,
      // No global validation layers for now.
      .enabledLayerCount = enableValidationLayers ? (uint32_t) validationLayers.size() : 0,
      .ppEnabledLayerNames = enableValidationLayers ? validationLayers.data() : nullptr,
      .enabledExtensionCount = (uint32_t) extensionInfo.size(),
      .ppEnabledExtensionNames = extensionInfo.data(),
    };

    // The general pattern that object creation function parameters in Vulkan follow is:
    // 1. Pointer to struct with creation info
    // 2. Pointer to custom allocator callbacks, always nullptr in this tutorial
    // 3. Pointer to the variable that stores the handle to the new object
    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/vkCreateInstance.html
    VkInstance instance;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS) {
      std::cerr << "vkCreateInstance failed with " << result << std::endl;
      throw std::runtime_error("failed to create instance!");
    }

    return instance;
  }

  /**
   * Returns the necessary Vulkan extension glfw needs.
   */
  ExtensionInfo glfwExtensionInfo() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    return ExtensionInfo {
      glfwExtensions, glfwExtensions + glfwExtensionCount,
    };
  }

  ExtensionInfo getDebugExtensions() {
    if (enableValidationLayers) {
      return { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
    }
    return { };
  }

  VkSurfaceKHR createSurface(VkInstance instance, GLFWwindow* window) {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
      throw std::runtime_error("failed to create window surface!");
    }

    return surface;
  }

  /**
   * Enumerate, select and return the best physical device available.
   */
  PhysicalDevice getPhysicalDevice(const VkInstance instance, const VkSurfaceKHR surface) {
    PhysicalDeviceEnumerator availablePhysicalDevices(instance, surface);

    if (availablePhysicalDevices.physicalDevices.size() <= 0) {
      throw std::runtime_error("No physical device present!");
    }

    for (auto device: availablePhysicalDevices.physicalDevices) {
      std::cout << device.properties.deviceName
                << " (vulkan " << VK_VERSION_MAJOR(device.properties.apiVersion) << "."
                << VK_VERSION_MINOR(device.properties.apiVersion) << "."
                << VK_VERSION_PATCH(device.properties.apiVersion) << " "
                << "vendorId:deviceId "
                << device.properties.vendorID << ":" << device.properties.deviceID << " "
                << "type " << device.properties.deviceType << ") " << std::endl;
    }

    // TODO: put here a check on the mandatory properties

    // Establish a score for each device through an heuristic and select the best device according
    // to that score.
    size_t bestDevice = 0;
    uint32_t bestScore = 0;
    for (size_t idx = 0; auto physicalDevice: availablePhysicalDevices.physicalDevices) {
      uint32_t score = 0;

      // We need the GPU to have a graphic queue...
      if (!physicalDevice.queueFamilyIndices.contains(VK_QUEUE_GRAPHICS_BIT)) {
        continue;
      }
      /// ... a queue able to manage a presentation state...
      if (physicalDevice.presentationStateQueueIndex == -1) {
        continue;
      }
      /// ... and to ha ve a swap chain.
      if (std::find_if(physicalDevice.availableExtensions.begin(),
                       physicalDevice.availableExtensions.end(),
                       [](auto &extension) {
                         return std::string(extension.extensionName) == VK_KHR_SWAPCHAIN_EXTENSION_NAME;
                       }
          ) == physicalDevice.availableExtensions.end()) {
        continue;
      }

      score += (physicalDevice.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        ? 1000 : 0;
      score += physicalDevice.properties.limits.maxImageDimension2D;

      if (score > bestScore) {
        bestScore = score;
        bestDevice = idx;
      }
    }

    return availablePhysicalDevices.physicalDevices[bestDevice];
  }

  /**
   * Creates and returns a logical device.
   */
  VkDevice getLogicalDevice(PhysicalDevice &physicalDevice) {
    float queuePriority = 1.0f;
    // The queues we need.
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
      physicalDevice.queueFamilyIndices[VK_QUEUE_GRAPHICS_BIT],
      // This could be equal to -1 but let's ignore that for the moment
      static_cast<uint32_t>(physicalDevice.presentationStateQueueIndex),
    };
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {
          .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
          .queueFamilyIndex = queueFamily,
          .queueCount = 1,
          .pQueuePriorities = &queuePriority,
        };
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // TODO: Also use this array when checking extension availablility in getPhysicalDevice.
    const std::vector<const char*> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // The needed features.
    VkPhysicalDeviceFeatures deviceFeatures = {};
    // Now the main logical device create structure.
    VkDeviceCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
      .pQueueCreateInfos = queueCreateInfos.data(),
      // Validation layers also needs to be specified when creating a logical device.
      .enabledLayerCount = enableValidationLayers ? (uint32_t) validationLayers.size() : 0,
      .ppEnabledLayerNames = enableValidationLayers ? validationLayers.data() : nullptr,
      .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
      .ppEnabledExtensionNames = deviceExtensions.data(),
      .pEnabledFeatures = &deviceFeatures,
    };
    VkDevice device;
    if (vkCreateDevice(physicalDevice.device, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    return device;
  }

  /**
   * Check all the provided validation layers are present on the system.
   */
  bool checkValidationLayerSupport(const std::vector<const char*> &validationLayers) {
    ValidationLayerEnumerator availableLayers;
    // For all the expected validation layers
    return std::all_of(validationLayers.begin(), validationLayers.end(), [&availableLayers](auto &layer) {
      // Check they are available
      auto it = std::find_if(availableLayers.layers.begin(), availableLayers.layers.end(),
        [&layer](auto &availableLayer) {
          return strcmp(layer, availableLayer.layerName) == 0;
        });
      return it != availableLayers.layers.end();
    });
  }

  /**
   * Display debug information about Vulkan.
   */
  void printVulkanBanner() {
    InstanceExtensionEnumerator extensionEnumerator;
    std::cout << extensionEnumerator.extensionCount << " available extensions:\n";
    for (const auto& extension : extensionEnumerator.extensions) {
      std::cout << '\t' << extension.extensionName << '\n';
    }

    ValidationLayerEnumerator availableLayers;
    std::cout << availableLayers.layerCount << " validation layers:\n";
    for (const auto& layer : availableLayers.layers) {
      std::cout << '\t' << layer.layerName << '\n';
    }
  }

  /**
   * Attach the debugCallback to Vulkan debug facility.
   */
  void setupDebugMessenger() {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                       | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                       | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                   | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                   | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = debugCallback,
      .pUserData = nullptr, // Optional
    };
    // The function to register the callback is an extension function and must be fetched dynamically
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(this->instance,
      "vkCreateDebugUtilsMessengerEXT");
    auto errorCode = (func != nullptr)
      ? func(this->instance, &createInfo, nullptr, &this->debugMessenger)
      : VK_ERROR_EXTENSION_NOT_PRESENT;
    if (errorCode != VK_SUCCESS) {
      throw std::runtime_error("failed to attach debug messenger!");
    }
  }

  void unSetupDebugMessenger() {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(this->instance,
      "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
      func(this->instance, this->debugMessenger, nullptr);
    }
  }

private:
  GLFWwindow* window = nullptr;
  VkInstance instance = VK_NULL_HANDLE;
  PhysicalDevice physicalDevice; // the vulkan physical device is a field of this structure.
  VkDevice device = VK_NULL_HANDLE;
  VkQueue graphicsQueue = VK_NULL_HANDLE;
  VkQueue presentQueue = VK_NULL_HANDLE;
  VkSurfaceKHR surface;

  VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
};

int main() {
  HelloTriangleApplication app;

  try {
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
