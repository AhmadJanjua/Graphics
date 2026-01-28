#include "Renderer.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <iostream>

// ----- CONSTANTS
constexpr uint32_t HEIGHT = 1000;
constexpr uint32_t WIDTH = 1000;
#ifdef NDEBUG
    constexpr bool ENABLE_VALIDATION = true;
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
        throw std::runtime_error("GLFW Error: failed to initialize");
    }

    glfwWindowHint(GLFW_CURSOR_HIDDEN, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Graphics App", nullptr, nullptr);

    if (window == nullptr) {
        throw std::runtime_error("GLFW Error: failed to create window");
    }
}

void Renderer::initVulkan() {
   createInstance();
   setupDebugMessenger();
}

void Renderer::createInstance() {
    constexpr vk::ApplicationInfo app_info {
        .pApplicationName   = "Graphics App",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "No Engine",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = vk::ApiVersion14
    };

    // Ensure necessary layers are enabled
    std::vector<char const*> required_layers;
    if (ENABLE_VALIDATION) {
        required_layers.assign(validation_layers.begin(), validation_layers.end());
    }

    auto supported_layers = context.enumerateInstanceLayerProperties();

    for (auto const &required_layer : required_layers) {
        if (std::ranges::none_of(
            supported_layers,
            [required_layer](auto const& supported) {
                return strcmp(required_layer, supported.layerName) == 0;
            }
        )) {
            throw std::runtime_error(
                    "Required layer not supported: "
                    + std::string(required_layer)
            );
        }
    }

    // Ensure that the necessary extensions are supported
    uint32_t properties_count = 0;
    auto glfw_properties = glfwGetRequiredInstanceExtensions(&properties_count);
    std::vector required_properties(glfw_properties, glfw_properties + properties_count);
    if (ENABLE_VALIDATION) {
        required_properties.push_back(vk::EXTDebugUtilsExtensionName);
    }
    auto supported_properties = context.enumerateInstanceExtensionProperties();

    for (auto const& required_property : required_properties) {
        if (std::ranges::none_of(
            supported_properties,
            [required_property](auto const &supported) {
                return strcmp(supported.extensionName, required_property) == 0;
            })
        ) {
            throw std::runtime_error(
                "Required GLFW extension not supported: "
                + std::string(required_property)
            );
		}
    }

    if (ENABLE_VALIDATION) {
        // available extensions
        std::cout << "Available Extensions:\n________" << std::endl;
        for (const auto &extension : supported_properties) {
            std::cout << extension.extensionName << std::endl;
        }
        std::cout << "_________" << std::endl;
    }

    vk::InstanceCreateInfo create_info{
        .pApplicationInfo        = &app_info,
        .enabledLayerCount       = static_cast<uint32_t>(required_layers.size()),
        .ppEnabledLayerNames     = required_layers.data(),
		.enabledExtensionCount   = static_cast<uint32_t>(required_properties.size()),
		.ppEnabledExtensionNames = required_properties.data()
    };

    instance = vk::raii::Instance(context, create_info);
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *
) {
    if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError ||
        severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
    ) {
		std::cerr
            << "validation layer: type "
            << to_string(type) << " msg: "
            << pCallbackData->pMessage
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
		.pfnUserCallback = &debugCallback
    };
	
    debug_messenger = instance.createDebugUtilsMessengerEXT(debug_create_info);
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
