cmake_minimum_required(VERSION 4.3)
project(VkHorizon)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
set(GLFW_USE_WAYLAND ON)


add_executable(VkHorizon src/main.cpp
        src/render/VulkanContext.cpp
        src/render/VulkanContext.h
        src/render/VulkanSwapchain.cpp
        src/render/VulkanSwapchain.h
        src/render/VulkanRenderer.cpp
        src/render/VulkanRenderer.h)

target_link_libraries(VkHorizon
        Vulkan::Vulkan
        glfw
)