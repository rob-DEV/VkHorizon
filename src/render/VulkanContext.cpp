#include "VulkanContext.h"

#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>

const std::vector<const char*> VulkanContext::validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> VulkanContext::deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifndef NDEBUG
static constexpr bool enableValidationLayers = true;
#else
static constexpr bool enableValidationLayers = false;
#endif

void VulkanContext::init(GLFWwindow* window)
{
    createInstance();
    setupDebugMessenger();
    createSurface(window);
    pickPhysicalDevice();
    createLogicalDevice();
}

void VulkanContext::cleanup()
{
    if (m_device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(m_device);
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }

    if (m_surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(
            m_instance,
            m_surface,
            nullptr
        );

        m_surface = VK_NULL_HANDLE;
    }

    if (enableValidationLayers &&
        m_debugMessenger != VK_NULL_HANDLE)
    {
        destroyDebugUtilsMessenger(
            m_instance,
            m_debugMessenger
        );

        m_debugMessenger = VK_NULL_HANDLE;
    }

    if (m_instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(
            m_instance,
            nullptr
        );

        m_instance = VK_NULL_HANDLE;
    }
}

bool VulkanContext::checkValidationLayerSupport()
{
    uint32_t layerCount = 0;

    vkEnumerateInstanceLayerProperties(
        &layerCount,
        nullptr
    );

    std::vector<VkLayerProperties> availableLayers(
        layerCount
    );

    vkEnumerateInstanceLayerProperties(
        &layerCount,
        availableLayers.data()
    );

    for (const char* layerName : validationLayers)
    {
        bool found = false;

        for (const auto& layer : availableLayers)
        {
            if (std::strcmp(
                    layerName,
                    layer.layerName) == 0)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            return false;
        }
    }

    return true;
}

void VulkanContext::createInstance()
{
    if (enableValidationLayers &&
        !checkValidationLayerSupport())
    {
        throw std::runtime_error(
            "Validation layers requested but unavailable"
        );
    }

    VkApplicationInfo appInfo{};

    appInfo.sType =
        VK_STRUCTURE_TYPE_APPLICATION_INFO;

    appInfo.pApplicationName =
        "VkHorizon";

    appInfo.applicationVersion =
        VK_MAKE_VERSION(1, 0, 0);

    appInfo.pEngineName =
        "VkHorizon";

    appInfo.engineVersion =
        VK_MAKE_VERSION(1, 0, 0);

    appInfo.apiVersion =
        VK_API_VERSION_1_3;

    uint32_t glfwExtensionCount = 0;

    const char** glfwExtensions =
        glfwGetRequiredInstanceExtensions(
            &glfwExtensionCount
        );

    if (!glfwExtensions)
    {
        throw std::runtime_error(
            "Failed to get GLFW Vulkan extensions"
        );
    }

    std::vector<const char*> extensions(
        glfwExtensions,
        glfwExtensions + glfwExtensionCount
    );

    if (enableValidationLayers)
    {
        extensions.push_back(
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        );
    }

    VkInstanceCreateInfo createInfo{};

    createInfo.sType =
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    createInfo.pApplicationInfo =
        &appInfo;

    createInfo.enabledExtensionCount =
        static_cast<uint32_t>(
            extensions.size()
        );

    createInfo.ppEnabledExtensionNames =
        extensions.data();

    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount =
            static_cast<uint32_t>(
                validationLayers.size()
            );

        createInfo.ppEnabledLayerNames =
            validationLayers.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

        populateDebugMessengerCreateInfo(
            debugCreateInfo
        );

        createInfo.pNext =
            &debugCreateInfo;
    }

    if (vkCreateInstance(
            &createInfo,
            nullptr,
            &m_instance) != VK_SUCCESS)
    {
        throw std::runtime_error(
            "Failed to create Vulkan instance"
        );
    }
}

void VulkanContext::populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};

    createInfo.sType =
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    createInfo.pfnUserCallback =
        debugCallback;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanContext::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData)
{
    std::cerr
        << "Validation: "
        << callbackData->pMessage
        << '\n';

    return VK_FALSE;
}

VkResult VulkanContext::createDebugUtilsMessenger(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
    VkDebugUtilsMessengerEXT* debugMessenger)
{
    auto func =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(
                instance,
                "vkCreateDebugUtilsMessengerEXT"
            )
        );

    if (func)
    {
        return func(
            instance,
            createInfo,
            nullptr,
            debugMessenger
        );
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void VulkanContext::destroyDebugUtilsMessenger(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger)
{
    auto func =
        reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(
                instance,
                "vkDestroyDebugUtilsMessengerEXT"
            )
        );

    if (func)
    {
        func(
            instance,
            debugMessenger,
            nullptr
        );
    }
}

void VulkanContext::setupDebugMessenger()
{
    if (!enableValidationLayers)
    {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};

    populateDebugMessengerCreateInfo(
        createInfo
    );

    if (createDebugUtilsMessenger(
            m_instance,
            &createInfo,
            &m_debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error(
            "Failed to create debug messenger"
        );
    }
}

void VulkanContext::createSurface(GLFWwindow* window)
{
    if (glfwCreateWindowSurface(
            m_instance,
            window,
            nullptr,
            &m_surface) != VK_SUCCESS)
    {
        throw std::runtime_error(
            "Failed to create window surface"
        );
    }
}

VulkanContext::QueueFamilyIndices
VulkanContext::findQueueFamilies(
    VkPhysicalDevice device) const
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(
        device,
        &queueFamilyCount,
        nullptr
    );

    std::vector<VkQueueFamilyProperties> queueFamilies(
        queueFamilyCount
    );

    vkGetPhysicalDeviceQueueFamilyProperties(
        device,
        &queueFamilyCount,
        queueFamilies.data()
    );

    for (uint32_t i = 0;
         i < queueFamilies.size();
         ++i)
    {
        const auto& queueFamily =
            queueFamilies[i];

        if (queueFamily.queueFlags &
            VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = VK_FALSE;

        vkGetPhysicalDeviceSurfaceSupportKHR(
            device,
            i,
            m_surface,
            &presentSupport
        );

        if (presentSupport)
        {
            indices.presentFamily = i;
        }

        if (indices.complete())
        {
            break;
        }
    }

    return indices;
}

void VulkanContext::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;

    vkEnumeratePhysicalDevices(
        m_instance,
        &deviceCount,
        nullptr
    );

    if (deviceCount == 0)
    {
        throw std::runtime_error(
            "Failed to find a GPU with Vulkan support"
        );
    }

    std::vector<VkPhysicalDevice> devices(
        deviceCount
    );

    vkEnumeratePhysicalDevices(
        m_instance,
        &deviceCount,
        devices.data()
    );

    for (VkPhysicalDevice device : devices)
    {
        VkPhysicalDeviceProperties properties{};

        vkGetPhysicalDeviceProperties(
            device,
            &properties
        );

        std::cout
            << "GPU: "
            << properties.deviceName
            << '\n';

        QueueFamilyIndices indices =
            findQueueFamilies(device);

        if (indices.complete())
        {
            m_physicalDevice = device;
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "Failed to find a suitable GPU"
        );
    }
}

void VulkanContext::createLogicalDevice()
{
    QueueFamilyIndices indices =
        findQueueFamilies(m_physicalDevice);

    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };

    float queuePriority = 1.0f;

    std::vector<VkDeviceQueueCreateInfo>
        queueCreateInfos;

    for (uint32_t queueFamily :
         uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};

        queueCreateInfo.sType =
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

        queueCreateInfo.queueFamilyIndex =
            queueFamily;

        queueCreateInfo.queueCount = 1;

        queueCreateInfo.pQueuePriorities =
            &queuePriority;

        queueCreateInfos.push_back(
            queueCreateInfo
        );
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};

    createInfo.sType =
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount =
        static_cast<uint32_t>(
            queueCreateInfos.size()
        );

    createInfo.pQueueCreateInfos =
        queueCreateInfos.data();

    createInfo.pEnabledFeatures =
        &deviceFeatures;

    createInfo.enabledExtensionCount =
        static_cast<uint32_t>(
            deviceExtensions.size()
        );

    createInfo.ppEnabledExtensionNames =
        deviceExtensions.data();

    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount =
            static_cast<uint32_t>(
                validationLayers.size()
            );

        createInfo.ppEnabledLayerNames =
            validationLayers.data();
    }

    if (vkCreateDevice(
            m_physicalDevice,
            &createInfo,
            nullptr,
            &m_device) != VK_SUCCESS)
    {
        throw std::runtime_error(
            "Failed to create logical device"
        );
    }

    m_graphicsQueueFamily =
    indices.graphicsFamily.value();

    m_presentQueueFamily =
        indices.presentFamily.value();

    vkGetDeviceQueue(
        m_device,
        indices.graphicsFamily.value(),
        0,
        &m_graphicsQueue
    );

    vkGetDeviceQueue(
        m_device,
        indices.presentFamily.value(),
        0,
        &m_presentQueue
    );
}