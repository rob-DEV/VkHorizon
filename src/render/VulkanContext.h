#ifndef VKHORIZON_VULKANCONTEXT_H
#define VKHORIZON_VULKANCONTEXT_H

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <optional>

class VulkanContext {
public:
    void init(GLFWwindow *window);

    void cleanup();

    VkInstance instance() const { return m_instance; }
    VkSurfaceKHR surface() const { return m_surface; }
    VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
    VkDevice device() const { return m_device; }

    VkQueue graphicsQueue() const { return m_graphicsQueue; }
    VkQueue presentQueue() const { return m_presentQueue; }

    uint32_t graphicsQueueFamily() const { return m_graphicsQueueFamily; }
    uint32_t presentQueueFamily() const { return m_presentQueueFamily; }

private:
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool complete() const {
            return graphicsFamily.has_value() &&
                   presentFamily.has_value();
        }
    };

    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;

    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;

    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;

    uint32_t m_graphicsQueueFamily = 0;
    uint32_t m_presentQueueFamily = 0;

    static const std::vector<const char *> validationLayers;
    static const std::vector<const char *> deviceExtensions;

    static bool checkValidationLayerSupport();

    static void populateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT &createInfo);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
        void *userData);

    static VkResult createDebugUtilsMessenger(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT *createInfo,
        VkDebugUtilsMessengerEXT *debugMessenger);

    static void destroyDebugUtilsMessenger(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger);

    void createInstance();

    void setupDebugMessenger();

    void createSurface(GLFWwindow *window);

    void pickPhysicalDevice();

    void createLogicalDevice();

    QueueFamilyIndices findQueueFamilies(
        VkPhysicalDevice device) const;
};

#endif //VKHORIZON_VULKANCONTEXT_H
