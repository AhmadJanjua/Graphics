#include "Renderer.h"
#include <GLFW/glfw3.h>
#include <stdexcept>

// --- Public
void Renderer::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

// --- Private
void Renderer::initWindow() {
    if (glfwInit() == GLFW_FALSE) {
        throw std::runtime_error("GLFW Error: failed to initialize");
    }

    glfwWindowHint(GLFW_CURSOR_HIDDEN, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Graphics", nullptr, nullptr);

    if (window == nullptr) {
        throw std::runtime_error("GLFW Error: failed to create window");
    }
}

void Renderer::initVulkan(){}

void Renderer::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

void Renderer::cleanup() {
    glfwDestroyWindow(window);
    glfwTerminate();
}
