#include "VulkanSwapchain.h"
#include "VulkanContext.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

void VulkanSwapchain::init(
    VulkanContext &context,
    GLFWwindow *window) {
    createSwapchain(
        context,
        window
    );

    createImageViews(
        context
    );
}

void VulkanSwapchain::cleanup(
    VulkanContext &context) {
    for (VkImageView imageView:
         m_imageViews) {
        vkDestroyImageView(
            context.device(),
            imageView,
            nullptr
        );
    }

    m_imageViews.clear();
    m_images.clear();

    if (m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(
            context.device(),
            m_swapchain,
            nullptr
        );

        m_swapchain =
                VK_NULL_HANDLE;
    }
}

VulkanSwapchain::SupportDetails
VulkanSwapchain::querySupport(
    VulkanContext &context) const {
    SupportDetails details;

    VkPhysicalDevice physicalDevice =
            context.physicalDevice();

    VkSurfaceKHR surface =
            context.surface();

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physicalDevice,
        surface,
        &details.capabilities
    );

    uint32_t formatCount = 0;

    vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice,
        surface,
        &formatCount,
        nullptr
    );

    if (formatCount > 0) {
        details.formats.resize(formatCount);

        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice,
            surface,
            &formatCount,
            details.formats.data()
        );
    }

    uint32_t presentModeCount = 0;

    vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice,
        surface,
        &presentModeCount,
        nullptr
    );

    if (presentModeCount > 0) {
        details.presentModes.resize(
            presentModeCount
        );

        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice,
            surface,
            &presentModeCount,
            details.presentModes.data()
        );
    }

    return details;
}

VkSurfaceFormatKHR VulkanSwapchain::chooseFormat(
    const std::vector<VkSurfaceFormatKHR> &formats
) const {
    for (const auto &format: formats) {
        if (format.format ==
            VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace ==
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    return formats[0];
}

VkPresentModeKHR VulkanSwapchain::choosePresentMode(
    const std::vector<VkPresentModeKHR> &modes
) const {
    for (const auto mode: modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::chooseExtent(
    const VkSurfaceCapabilitiesKHR &capabilities,
    GLFWwindow *window
) const {
    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width;
    int height;

    glfwGetFramebufferSize(
        window,
        &width,
        &height
    );

    VkExtent2D extent{
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    extent.width = std::clamp(
        extent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width
    );

    extent.height = std::clamp(
        extent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height
    );

    return extent;
}


void VulkanSwapchain::createSwapchain(
    VulkanContext &context,
    GLFWwindow *window) {
    SupportDetails support =
            querySupport(context);

    VkSurfaceFormatKHR surfaceFormat =
            chooseFormat(support.formats);

    VkPresentModeKHR presentMode =
            choosePresentMode(support.presentModes);

    VkExtent2D extent =
            chooseExtent(
                support.capabilities,
                window
            );

    uint32_t imageCount =
            support.capabilities.minImageCount + 1;

    if (support.capabilities.maxImageCount > 0 &&
        imageCount >
        support.capabilities.maxImageCount) {
        imageCount =
                support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};

    createInfo.sType =
            VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

    createInfo.surface =
            context.surface();

    createInfo.minImageCount =
            imageCount;

    createInfo.imageFormat =
            surfaceFormat.format;

    createInfo.imageColorSpace =
            surfaceFormat.colorSpace;

    createInfo.imageExtent =
            extent;

    createInfo.imageArrayLayers = 1;

    createInfo.imageUsage =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {
        context.graphicsQueueFamily(),
        context.presentQueueFamily()
    };

    if (context.graphicsQueueFamily() !=
        context.presentQueueFamily()) {
        createInfo.imageSharingMode =
                VK_SHARING_MODE_CONCURRENT;

        createInfo.queueFamilyIndexCount = 2;

        createInfo.pQueueFamilyIndices =
                queueFamilyIndices;
    } else {
        createInfo.imageSharingMode =
                VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform =
            support.capabilities.currentTransform;

    createInfo.compositeAlpha =
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode =
            presentMode;

    createInfo.clipped =
            VK_TRUE;

    createInfo.oldSwapchain =
            VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(
            context.device(),
            &createInfo,
            nullptr,
            &m_swapchain) != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create swapchain"
        );
    }

    m_imageFormat =
            surfaceFormat.format;

    m_extent =
            extent;

    vkGetSwapchainImagesKHR(
        context.device(),
        m_swapchain,
        &imageCount,
        nullptr
    );

    m_images.resize(imageCount);

    vkGetSwapchainImagesKHR(
        context.device(),
        m_swapchain,
        &imageCount,
        m_images.data()
    );
}

void VulkanSwapchain::createImageViews(
    VulkanContext &context) {
    m_imageViews.resize(
        m_images.size()
    );

    for (size_t i = 0;
         i < m_images.size();
         ++i) {
        VkImageViewCreateInfo createInfo{};

        createInfo.sType =
                VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

        createInfo.image =
                m_images[i];

        createInfo.viewType =
                VK_IMAGE_VIEW_TYPE_2D;

        createInfo.format =
                m_imageFormat;

        createInfo.components.r =
                VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.components.g =
                VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.components.b =
                VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.components.a =
                VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask =
                VK_IMAGE_ASPECT_COLOR_BIT;

        createInfo.subresourceRange.baseMipLevel =
                0;

        createInfo.subresourceRange.levelCount =
                1;

        createInfo.subresourceRange.baseArrayLayer =
                0;

        createInfo.subresourceRange.layerCount =
                1;

        if (vkCreateImageView(
                context.device(),
                &createInfo,
                nullptr,
                &m_imageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to create swapchain image view"
            );
        }
    }
}
