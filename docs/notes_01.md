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

# Instance
- this defines the vulkan library
- to use RAII we must use api >= 1.4

# Validation Layer
- To ensure low overhead, validation needs to implemented by the use
- the layer can display
    - evaluate the parameters
    - track object lifecycle
    - logging to STDIO 
    - allows profiling and replaying
- Can use lunarG validation layer to save time
    - only works if the user has the sdk installed

# Physical Devices
- Vulkan shows all the available devices that support it 
- also allows you to evaluate available hardware and select what is best

# Queue Families
- removes the abstraction of jobs submitted to the GPU
- exposes the different engines available to you
- queues for 
    - graphics
    - compute
    - transfer
    - presentation 
- you can parse devices to see if it supports what you're looking for and grade the hardware based on that

# Logical Devices and Queues  
- These are software level instances of the actual hardware 
- to gain an instance of hardware and available queues on the hardware you need to describe what you will use from them
- then you are given an instance of the devices to send information to

# Window Surface
- vulkan is windowing system agnostic
- must be tethered to a windowing system
- these are instance level extensions that have been enabled when we created an instance of vulkan
- additionally we must ensure to make a presentation queue to present to the surface

# Swap Chains
- no default framebuffer -> implemented via swap chain 
- server specific gpus may not have display capabilities, hence no swap chain extension 

# Image View 
- to use images in the swap chain we need to create an image view
- image view describes what portion of the image is viewed. 
