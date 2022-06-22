add_executable(toy
    app.cpp
    bt_device.cpp
    bt_filesystem.cpp
    bt_logger.cpp
    bt_model.cpp
    bt_pipeline.cpp
    bt_swapchain.cpp
    bt_window.cpp
    main.cpp)

target_include_directories(toy PUBLIC .)

target_link_libraries(toy PRIVATE fmt::fmt glad_vulkan_12 glfw glm spdlog::spdlog)

add_dependencies(toy shaders)

# add_custom_command(TARGET toy POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_BINARY_DIR}/src/shaders)

# ############# Build SHADERS #######################

# Find all vertex and fragment sources within shaders directory
# taken from VBlancos vulkan tutorial
# https://github.com/vblanco20-1/vulkan-guide/blob/all-chapters/CMakeLists.txt
find_program(GLSLC glslc HINTS
    ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
    /usr/bin
    /usr/local/bin
    ${VULKAN_SDK_PATH}/Bin
    ${VULKAN_SDK_PATH}/Bin32
    $ENV{VULKAN_SDK}/Bin/
    $ENV{VULKAN_SDK}/Bin32/
)

# get all .vert and .frag files in shaders directory
file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/shaders/*.glsl"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME_WLE)
    get_filename_component(SHADER_EXT ${FILE_NAME} LAST_EXT)
    string(SUBSTRING ${SHADER_EXT} 1 -1 SHADER_TYPE) # get_filename_component last_ext includes the period; remove it.
    set(SPIRV "${PROJECT_BINARY_DIR}/bin/shaders/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${GLSLC} -fshader-stage=${SHADER_TYPE} -o ${SPIRV} ${GLSL}
        DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach()

add_custom_target(
    shaders
    DEPENDS ${SPIRV_BINARY_FILES}
)
