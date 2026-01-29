#include "Renderer.h"
#include "vulkan/vulkan.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>
#include <iostream>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

// ----- PUBLIC
void Renderer::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

// ----- PRIVATE
void Renderer::initWindow() {
    if (glfwInit() == GLFW_FALSE) {
        throw std::runtime_error("GLFW failed to initialize");
    }

    if (ENABLE_VALIDATION) {
        std::cerr << "GLFW platform: " << glfwGetPlatform() << "\n";
    }

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Graphics App", nullptr, nullptr);

    if (window == nullptr) {
        throw std::runtime_error("GLFW failed to create window");
    }
}

void Renderer::initVulkan() {
   createInstance();
   setupDebugMessenger();
   createSurface();
   pickPhysicalDevice();
   createLogicalDevice();
}

void Renderer::createInstance() {
    constexpr vk::ApplicationInfo app_info {
        .pApplicationName   = "Graphics App",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "No Engine",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = vk::ApiVersion14
    };

    // Add validation layer if debugging is active
    std::vector<char const*> required_layers;
    if (ENABLE_VALIDATION) {
        required_layers.assign(validation_layers.begin(), validation_layers.end());
    }

    auto supported_layers = context.enumerateInstanceLayerProperties();

    if (ENABLE_VALIDATION) {
        std::cerr << "Supported layers:" << std::endl;
        for (auto &layer : supported_layers) {
            std::cerr << "-\t" << layer.layerName << std::endl;
        }
        std::cerr << std::endl;
    }

    for (auto const &required : required_layers) {
        if (std::ranges::none_of(
            supported_layers,
            [required](auto const& supported) {
                return strcmp(required, supported.layerName) == 0;
            }
        )) {
            throw std::runtime_error(
                    "Required layer not supported: "
                    + std::string(required)
            );
        }
    }

    // Ensure that the necessary extensions are supported
    uint32_t extension_count = 0;
    auto glfw_extensions = glfwGetRequiredInstanceExtensions(&extension_count);

    // add LunarG validation layer for debugging
    std::vector required_extensions(glfw_extensions, glfw_extensions + extension_count);
    if (ENABLE_VALIDATION) {
        required_extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    auto supported_extensions = context.enumerateInstanceExtensionProperties();

    if (ENABLE_VALIDATION) {
        std::cerr << "Available Extensions:" << std::endl;
        for (const auto &extension : supported_extensions) {
            std::cerr << "-\t" << extension.extensionName << std::endl;
        }
        std::cerr << std::endl;
    }

    for (auto const& required : required_extensions) {
        if (std::ranges::none_of(
            supported_extensions,
            [required](auto const &supported) {
                return strcmp(required, supported.extensionName) == 0;
            })
        ) {
            throw std::runtime_error(
                "Required extension not supported: "
                + std::string(required)
            );
		}
    }

    vk::InstanceCreateInfo create_info {
        .pApplicationInfo        = &app_info,
        .enabledLayerCount       = static_cast<uint32_t>(required_layers.size()),
        .ppEnabledLayerNames     = required_layers.data(),
		.enabledExtensionCount   = static_cast<uint32_t>(required_extensions.size()),
		.ppEnabledExtensionNames = required_extensions.data()
    };

    instance = vk::raii::Instance(context, create_info);
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCB(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *
) {
    if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError ||
        severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
    ) {
		std::cerr
            << "validation layer: type " << to_string(type)
            << " msg: " << pCallbackData->pMessage
            << std::endl;
	}
	return vk::False;
}

void Renderer::setupDebugMessenger() {
    if (!ENABLE_VALIDATION) return;

    vk::DebugUtilsMessageSeverityFlagsEXT severity_flags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
    );
	vk::DebugUtilsMessageTypeFlagsEXT message_flags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
    );
	vk::DebugUtilsMessengerCreateInfoEXT debug_create_info {
        .messageSeverity = severity_flags,
		.messageType     = message_flags,
		.pfnUserCallback = &debugCB
    };
	
    debug_messenger = instance.createDebugUtilsMessengerEXT(debug_create_info);
}

void Renderer::createSurface() {
    // convert c -> c++ binding
    VkSurfaceKHR _surface;
    auto result = glfwCreateWindowSurface(*instance, window, nullptr, &_surface);

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
                "Failed to create surface with error code: "
                + std::to_string(result)
        );
    }

    surface = vk::raii::SurfaceKHR(instance, _surface);
}

void Renderer::pickPhysicalDevice() {
    auto devices = instance.enumeratePhysicalDevices();

    if (devices.empty()) {
        throw std::runtime_error("No physical devices available");
    }

    std::multimap<uint32_t, vk::raii::PhysicalDevice> scored_devices;

    for (auto &device : devices) {
        auto properties = device.getProperties();
        auto features = device.getFeatures();
        auto extensions = device.enumerateDeviceExtensionProperties();
        auto queue_families = device.getQueueFamilyProperties();
        uint32_t score = 0;

        // Score the GPUs
        if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            score += 1000;
        }
        score += properties.limits.maxImageDimension2D;

        // --- Compatibility
        if (!features.geometryShader) score = 0;
        if (properties.apiVersion < VK_API_VERSION_1_4) score = 0;

        if (std::ranges::none_of(
            queue_families,
            [](auto const& family) {
                return (family.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
            })
        ) {
            score = 0;
        }
        for (auto &required : device_extensions) {
            if (std::ranges::none_of(
                extensions,
                [required](auto const &supported) {
                    return strcmp(required, supported.extensionName) == 0;
                })
            ) {
                score = 0;
                break;
            }
        }

        if (ENABLE_VALIDATION) {
            std::cerr
                << "Physical Device: " << properties.deviceName
                << "\tScore: " << score
                << std::endl;
        }

        if (score == 0) continue;

        scored_devices.insert(std::make_pair(score, device));       
    }

    if (scored_devices.empty()) {
        throw std::runtime_error("No suitable physical device available");
    }

    physical_device = scored_devices.rbegin()->second;
}

void Renderer::createLogicalDevice() {
   auto family_properties = physical_device.getQueueFamilyProperties();

   const uint32_t max_idx = family_properties.size();
   uint32_t gfx_idx = max_idx;
   uint32_t pres_idx = max_idx;

   // try to find queue that can do both graphics and presentation
   // fallback is using the first different queues that support these properties
   for (uint32_t i = 0; i < max_idx; i++) {
       bool valid_gfx = bool(family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics);
       bool valid_pres = physical_device.getSurfaceSupportKHR(i, *surface);

       if (valid_gfx && valid_pres) {
           gfx_idx = i;
           pres_idx = i;
           break;
       } else if (valid_gfx && gfx_idx == max_idx) {
           gfx_idx = i;
       } else if (valid_pres && pres_idx == max_idx) {
           pres_idx = i;
       }
   }

   if (gfx_idx == max_idx) {
       throw std::runtime_error("No graphics queue available");
   }
   if (pres_idx == max_idx) {
       throw std::runtime_error("No presentation queue available");
   }

   vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
    feature_chain = {
        {},
		{.dynamicRendering = true},
		{.extendedDynamicState = true}
	};

   float priority = 0.5f;
   vk::DeviceQueueCreateInfo queue_info = {
       .queueFamilyIndex = gfx_idx,
       .queueCount       = 1,
       .pQueuePriorities = &priority
   };

   vk::DeviceCreateInfo device_info = {
       .pNext = &feature_chain.get<vk::PhysicalDeviceFeatures2>(),
       .queueCreateInfoCount    = 1,
       .pQueueCreateInfos       = &queue_info,
       .enabledExtensionCount   = static_cast<uint32_t>(device_extensions.size()),
       .ppEnabledExtensionNames = device_extensions.data()
   };

   logical_device = vk::raii::Device(physical_device, device_info);
   gfx_queue = vk::raii::Queue(logical_device, gfx_idx, 0);
   pres_queue = vk::raii::Queue(logical_device, pres_idx, 0);
}

void Renderer::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

void Renderer::cleanup() {
    glfwDestroyWindow(window);
    glfwTerminate();
}
