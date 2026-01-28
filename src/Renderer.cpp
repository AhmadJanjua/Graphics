#include "Renderer.h"
#include "vulkan/vulkan.hpp"
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

// ----- CONSTANTS
constexpr uint32_t HEIGHT = 1000;
constexpr uint32_t WIDTH = 1000;
#ifdef NDEBUG
    constexpr bool ENABLE_VALIDATION = false;
#else
    constexpr bool ENABLE_VALIDATION = true;
#endif


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

    glfwWindowHint(GLFW_CURSOR_HIDDEN, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Graphics App", nullptr, nullptr);

    if (window == nullptr) {
        throw std::runtime_error("GLFW failed to create window");
    }
}

void Renderer::initVulkan() {
   createInstance();
   setupDebugMessenger();
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
        std::cout << "Supported layers:" << std::endl;
        for (auto &layer : supported_layers) {
            std::cout << "-\t" << layer.layerName << std::endl;
        }
        std::cout << std::endl;
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
        std::cout << "Available Extensions:" << std::endl;
        for (const auto &extension : supported_extensions) {
            std::cout << "-\t" << extension.extensionName << std::endl;
        }
        std::cout << std::endl;
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
        if (!features.geometryShader) {
            score = 0;
        }
        if (properties.apiVersion < VK_API_VERSION_1_4) {
            score = 0;
        }
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
            std::cout
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

   // find graphics queue family
   auto gfx_property = std::ranges::find_if(
            family_properties,
            [](const auto& property) {
                return (property.queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlags(0);
            });
   if (gfx_property == family_properties.end()) {
       throw std::runtime_error("No graphics queue family found");
   }
   uint32_t gfx_idx = std::distance(family_properties.begin(), gfx_property);

	vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    > feature_chain = {
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
