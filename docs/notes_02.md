# Graphics Pipeline
- One of the main reasons to use vulkan over OpenGL is how it exposes the graphics pipeline!

## Overview
- The pipeline is as follows

1. Vertex/Index buffers
    - this includes the geometry that needs to be drawn
    - vertex are the raw data holders
    - indexes are pointers to the vertex - for space efficiency
2. Input Assembler (FIXED)
    - collects the vertex and index data from the buffers directly
    - retrieval step that reconciles the indexes
3. Vertex Shader
    - runs for every vertex
    - general use is to apply vertex transformations
    - model space -> screen space
4. Tesselation Shader
    - Used to subdivide geometry to increase mesh quality using some rules
5. Geometry Shader
    - runs actions on every single primitive (triangle, line, point)
    - more flexible than tesselation shader but similar
    - not great performance except on intel igpus
    - modern mesh shader pipeline subverts using this for performance gains
6. Rasterization (FIXED)
    - primitives are converted to points on the frame buffer 
    - discards elements that are outside of the view and behind others (depth test)
7. Fragment Shader
    - runs for every fragment
    - determines the color and depth
    - used to display lighting and textures generally
8. Color Blending (FIXED)
    - Determines how overlapping primitive to the same fragment behave (addative, transparent etc)

Fixed stages allow them to be tweaked but behaviour is predetermined. Other stages are hand programmed with user features.

In OpenGL you can change pipeline behaviours on the fly, but on vulkan, once created, the functionality is fixed.

## Shader Modules
- Shader code must be written in SPIRV byte-code for vulkan to understand
- different from HLSL, GLSL, Slang
- reduces the complexity of the GPU vendor compilers and reduces compilation erros on code written between vendors
- Khronos has a Slang to SPIRV compiler 
- *swizzle operator:* you can access a single component of a vector using .x, .xy, .z, etc.

### Vertex Shader
- takes in vertices along with attributes such as position, normal, color, and texture coordinates + more
- makes use of the normalized coordinate system (-1, 1), with the y coordinate flipped -1 is the top
- 
