cmake_minimum_required (VERSION 4.2)
project (Graphics)

# files that help find packages
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake")


# Add option to enable/disable C++ 20 module
option(ENABLE_CPP20_MODULE "Enable C++ 20 module support for Vulkan" OFF)

# Enable C++ module dependency scanning only if C++ 20 module is enabled
if(ENABLE_CPP20_MODULE)
    set(CMAKE_CXX_SCAN_FOR_MODULES ON)
endif()


# import necessary packages
find_package (glfw3 REQUIRED)
find_package (glm REQUIRED)
find_package (Vulkan 1.4.335 REQUIRED)
find_package (tinyobjloader REQUIRED)
find_package (tinygltf REQUIRED)
find_package (KTX REQUIRED)
find_package(stb REQUIRED)
set(STB_INCLUDEDIR ${stb_INCLUDE_DIRS})


# set up Vulkan C++ module only if enabled
if(ENABLE_CPP20_MODULE)
    add_library(VulkanCppModule)
    add_library(Vulkan::cppm ALIAS VulkanCppModule)

    target_compile_definitions(VulkanCppModule
          PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
                 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    )
    target_include_directories(VulkanCppModule
          PUBLIC "${Vulkan_INCLUDE_DIR}"
    )
    target_link_libraries(VulkanCppModule
          PUBLIC Vulkan::Vulkan
    )
    set_target_properties(VulkanCppModule 
        PROPERTIES CXX_STANDARD 20
    )

    # Add MSVC-specific compiler options for proper C++ module support
    if(MSVC)
        target_compile_options(VulkanCppModule PRIVATE
            # Use latest C++ standard for better module support
            /std:c++latest      
            # Standards conformance mode
            /permissive-        
            # Enable correct __cplusplus macro
            /Zc:__cplusplus     
            # Enable C++ exception handling
            /EHsc               
            # Use conforming preprocessor
            /Zc:preprocessor
            # Automatically translate #include to import for standard library
            /translateInclude
        )
    endif()

    target_sources(VulkanCppModule
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS "${Vulkan_INCLUDE_DIR}"
        FILES "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
    )

    # Add the vulkan.cppm file directly as a source file
    target_sources(VulkanCppModule
        PRIVATE "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
    )
else()
    # Create a dummy interface library when C++ 20 module is disabled
    add_library(VulkanCppModule
        INTERFACE
    )
    add_library(Vulkan::cppm
        ALIAS VulkanCppModule
    )
    target_link_libraries(VulkanCppModule
        INTERFACE Vulkan::Vulkan
    )
    target_compile_definitions(VulkanCppModule
        INTERFACE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
                  VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    )
endif()


# shader compilation tools
add_executable (glslang::validator IMPORTED)
find_program (
    GLSLANG_VALIDATOR "glslangValidator"
    HINTS $ENV{VULKAN_SDK}/bin REQUIRED
)
set_property (
    TARGET glslang::validator
    PROPERTY IMPORTED_LOCATION "${GLSLANG_VALIDATOR}"
)
find_program(
    SLANGC_EXECUTABLE slangc
    HINTS $ENV{VULKAN_SDK}/bin REQUIRED
)


function (add_shaders_target TARGET)
  cmake_parse_arguments ("SHADER" "" "CHAPTER_NAME" "SOURCES" ${ARGN})
  set (SHADERS_DIR ${SHADER_CHAPTER_NAME}/shaders)
  add_custom_command (
    OUTPUT ${SHADERS_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
    )
  add_custom_command (
    OUTPUT ${SHADERS_DIR}/frag.spv ${SHADERS_DIR}/vert.spv
    COMMAND glslang::validator
    ARGS --target-env vulkan1.0 ${SHADER_SOURCES} --quiet
    WORKING_DIRECTORY ${SHADERS_DIR}
    DEPENDS ${SHADERS_DIR} ${SHADER_SOURCES}
    COMMENT "Compiling Shaders"
    VERBATIM
    )
  add_custom_target (${TARGET} DEPENDS ${SHADERS_DIR}/frag.spv ${SHADERS_DIR}/vert.spv)
endfunction ()

function (add_slang_shader_target TARGET)
  cmake_parse_arguments ("SHADER" "" "CHAPTER_NAME" "SOURCES" ${ARGN})
  set (SHADERS_DIR ${SHADER_CHAPTER_NAME}/shaders)
  file(GLOB HAS_COMPUTE ${CHAPTER_SHADER}.comp)
  set (ENTRY_POINTS -entry vertMain -entry fragMain)
  if(HAS_COMPUTE)
    list(APPEND ENTRY_POINTS -entry compMain)
  endif()
  add_custom_command (
          OUTPUT ${SHADERS_DIR}
          COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
  )
  add_custom_command (
          OUTPUT  ${SHADERS_DIR}/slang.spv
          COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES} -target spirv -profile spirv_1_4+spvRayQueryKHR -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o slang.spv
          WORKING_DIRECTORY ${SHADERS_DIR}
          DEPENDS ${SHADERS_DIR} ${SHADER_SOURCES}
          COMMENT "Compiling Slang Shaders"
          VERBATIM
  )
  add_custom_target (${TARGET} DEPENDS ${SHADERS_DIR}/slang.spv)
endfunction()

function (add_app TARGET_NAME)
  cmake_parse_arguments (APP "" "SHADER" "SOURCES;LIBS;TEXTURES;MODELS" ${ARGN})

  add_executable (${TARGET_NAME} ${APP_SOURCES})
  set_target_properties (${TARGET_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${TARGET_NAME})
  set_target_properties (${TARGET_NAME} PROPERTIES CXX_STANDARD 20)

  target_link_libraries (${TARGET_NAME} Vulkan::cppm glfw)
  target_include_directories (${TARGET_NAME} PRIVATE ${STB_INCLUDEDIR} "${CMAKE_SOURCE_DIR}/include")

  if(ENABLE_CPP20_MODULE)
    target_compile_definitions(${TARGET_NAME} PRIVATE USE_CPP20_MODULES=1)
  endif()

  target_compile_definitions(${TARGET_NAME} PRIVATE VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS)

  if(WIN32 AND ${CMAKE_GENERATOR} MATCHES "Visual Studio.*")
    set_target_properties(${TARGET_NAME} PROPERTIES
      VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${TARGET_NAME}")
  endif()

  # shaders
  if (DEFINED APP_SHADER)
    set (SHADER_TARGET ${TARGET_NAME}_shader)
    file (GLOB SHADER_SOURCES
      "${APP_SHADER}.frag"
      "${APP_SHADER}.vert"
      "${APP_SHADER}.comp"
    )
    add_shaders_target (${SHADER_TARGET} CHAPTER_NAME ${TARGET_NAME} SOURCES ${SHADER_SOURCES})
    add_dependencies (${TARGET_NAME} ${SHADER_TARGET})

    set (SLANG_TARGET ${TARGET_NAME}_slang_shader)
    file (GLOB SHADER_SLANG_SOURCES "${APP_SHADER}.slang")
    if(SHADER_SLANG_SOURCES)
      add_slang_shader_target (${SLANG_TARGET} CHAPTER_NAME ${TARGET_NAME} SOURCES ${SHADER_SLANG_SOURCES})
      add_dependencies (${TARGET_NAME} ${SLANG_TARGET})
    endif()
  endif ()

  if (DEFINED APP_LIBS)
    target_link_libraries (${TARGET_NAME} ${APP_LIBS})
  endif ()

  if (DEFINED APP_MODELS)
    list(TRANSFORM APP_MODELS PREPEND "${CMAKE_SOURCE_DIR}/assets/")
    file (COPY ${APP_MODELS} DESTINATION ${CMAKE_BINARY_DIR}/${TARGET_NAME}/models)
  endif ()

  if (DEFINED APP_TEXTURES)
    list(TRANSFORM APP_TEXTURES PREPEND "${CMAKE_SOURCE_DIR}/assets/")
    file (COPY ${APP_TEXTURES} DESTINATION ${CMAKE_BINARY_DIR}/${TARGET_NAME}/textures)
  endif ()
endfunction ()
add_app(Graphics
  SOURCES
    src/main.cpp
  SHADER
    "${CMAKE_SOURCE_DIR}/shaders/main"
)

