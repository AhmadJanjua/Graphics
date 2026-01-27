# About
This document is mainly for me to keep track of important concepts.

# Overview
## Purpose of Vulkan
- Targets modern hardware
- Reduces driver overhead (must know what you're doing)
- Provides better multi-threading support (threads can submit commands in parallel)
- Standardized shader byte code
- 
## Coding Conventions
- functions - vk
- Types/Structs - Vk
- Enumerations - VK_

## How to draw a triangle
1. Create a Vulkan Instance
2. Select a physical device
3. Select a logical device - define what you will be using from the physical device
4. Create a queue family - perform async operations and get retrieve them from queue
5. Create window with GLFW or SDL and a vulkan surface
6. Implement a swap chain (have multiple surfaces - to show one, while we render another in the background)
7. Image view and dynamic rendering - on the fly description of what rendering passes to complete
8. Create a graphics pipeline - all pipelines must be defined in advance.
9. Command buffers - sends a command to the gpu that it will return in the queue family
10. Main loop: aquire image -> send to command buffer -> send to swap chain for presentation

# Setup
- This project makes heavy use of Vulkan RAII functions to simplify resource management
- Otherwise we would need to VkCreate/VkAllocate and VkFree/VkDestroy resources as needed

