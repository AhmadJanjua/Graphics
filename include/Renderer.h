#pragma once

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Renderer {
public:
    void run();

private:
    void initWindow();

    void initVulkan();
    void createInstance();
    void setupDebugMessenger();
    void pickPhysicalDevice();
    void createLogicalDevice();

    void mainLoop();
    
    void cleanup();


    GLFWwindow* window = nullptr;

    const std::vector<char const*> validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };
    const std::vector<const char *> device_extensions = {
        vk::KHRSwapchainExtensionName
    };


    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;

    vk::raii::DebugUtilsMessengerEXT debug_messenger = nullptr;

    vk::raii::PhysicalDevice physical_device = nullptr;
    vk::raii::Device logical_device = nullptr;

    vk::raii::Queue gfx_queue = nullptr;
};
