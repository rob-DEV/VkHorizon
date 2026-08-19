#include <GLFW/glfw3.h>

#include <iostream>
#include <optional>

#include "render/VulkanContext.h"
#include "render/VulkanRenderer.h"
#include "render/VulkanSwapchain.h"


int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow *window = glfwCreateWindow(
        1280,
        720,
        "VkHorizon",
        nullptr,
        nullptr
    );

    if (!window) {
        glfwTerminate();
        return 1;
    }

    try {
        VulkanContext context;

        context.init(window);

        VulkanSwapchain swapchain;

        swapchain.init(
            context,
            window
        );

        VulkanRenderer renderer;
        renderer.init(context, swapchain);

        std::cout
                << "Swapchain created: "
                << swapchain.extent().width
                << "x"
                << swapchain.extent().height
                << '\n';

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            renderer.drawFrame(
                context,
                swapchain
            );
        }

        renderer.cleanup(context);
        swapchain.cleanup(context);
        context.cleanup();
    } catch (const std::exception &e) {
        std::cerr
                << e.what()
                << '\n';

        glfwDestroyWindow(window);
        glfwTerminate();

        return 1;
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
