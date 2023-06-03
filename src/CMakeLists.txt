
# Add source to this project's executable.
add_executable(cpp-vulkan
        main.cpp
        VulkanEngine.cpp
        VulkanEngine.h
        VulkanTypes.h
        VulkanInitialisers.cpp
        VulkanInitialisers.h PipelineBuilder.cpp PipelineBuilder.h)


set_property(TARGET cpp-vulkan PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:cpp-vulkan>")

target_include_directories(cpp-vulkan PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(cpp-vulkan vkbootstrap vma glm tinyobjloader imgui stb_image)

target_link_libraries(cpp-vulkan Vulkan::Vulkan sdl2)

add_dependencies(cpp-vulkan Shaders)
