//
// Created by robert on 18/08/2026.
//

#ifndef VKHORIZON_VULKANSWAPCHAIN_H
#define VKHORIZON_VULKANSWAPCHAIN_H
#include <vector>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include "VulkanContext.h"


#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <vector>

class VulkanContext;

class VulkanSwapchain
{
public:
    void init(
        VulkanContext& context,
        GLFWwindow* window
    );

    void cleanup(
        VulkanContext& context
    );

    VkSwapchainKHR handle() const
    {
        return m_swapchain;
    }

    VkFormat imageFormat() const
    {
        return m_imageFormat;
    }

    VkExtent2D extent() const
    {
        return m_extent;
    }

    const std::vector<VkImage>& images() const
    {
        return m_images;
    }

    const std::vector<VkImageView>& imageViews() const
    {
        return m_imageViews;
    }

private:
    struct SupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities{};

        std::vector<VkSurfaceFormatKHR> formats;

        std::vector<VkPresentModeKHR> presentModes;
    };

    SupportDetails querySupport(
        VulkanContext& context
    ) const;

    VkSurfaceFormatKHR chooseFormat(
        const std::vector<VkSurfaceFormatKHR>& formats
    ) const;

    VkPresentModeKHR choosePresentMode(
        const std::vector<VkPresentModeKHR>& modes
    ) const;

    VkExtent2D chooseExtent(
        const VkSurfaceCapabilitiesKHR& capabilities,
        GLFWwindow* window
    ) const;

    void createSwapchain(
        VulkanContext& context,
        GLFWwindow* window
    );

    void createImageViews(
        VulkanContext& context
    );

private:
    VkSwapchainKHR m_swapchain =
        VK_NULL_HANDLE;

    VkFormat m_imageFormat{};

    VkExtent2D m_extent{};

    std::vector<VkImage> m_images;

    std::vector<VkImageView> m_imageViews;
};

#endif //VKHORIZON_VULKANSWAPCHAIN_H
