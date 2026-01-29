#pragma once

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// ----- CONSTANTS
constexpr uint32_t HEIGHT = 1000;
constexpr uint32_t WIDTH = 1000;
#ifdef NDEBUG
    constexpr bool ENABLE_VALIDATION = false;
#else
    constexpr bool ENABLE_VALIDATION = true;
#endif

class Renderer {
public:
    void run();

private:
    void initWindow();

    void initVulkan();
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();

    void mainLoop();
    
    void cleanup();

    const std::vector<char const*> validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };
    const std::vector<const char *> device_extensions = {
        vk::KHRSwapchainExtensionName
    };

    GLFWwindow* window = nullptr;

    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;

    vk::raii::DebugUtilsMessengerEXT debug_messenger = nullptr;

    vk::raii::SurfaceKHR surface = nullptr;

    vk::raii::PhysicalDevice physical_device = nullptr;
    vk::raii::Device logical_device = nullptr;

    vk::raii::Queue gfx_queue = nullptr;
    vk::raii::Queue pres_queue = nullptr;

};
