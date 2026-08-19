//
// Created by robert on 18/08/2026.
//

#ifndef VKHORIZON_VULKANRENDERER_H
#define VKHORIZON_VULKANRENDERER_H
#include "VulkanContext.h"
#include "VulkanSwapchain.h"

#include <vulkan/vulkan.h>

class VulkanContext;
class VulkanSwapchain;

class VulkanRenderer {
public:
    void init(
        VulkanContext &context,
        VulkanSwapchain &swapchain
    );

    void drawFrame(
        VulkanContext& context,
        VulkanSwapchain& swapchain
    );

    void cleanup(
        VulkanContext &context
    );

private:
    void createRenderPass(
        VulkanContext &context,
        VulkanSwapchain &swapchain
    );

    void createGraphicsPipeline(
        VulkanContext &context,
        VulkanSwapchain &swapchain
    );

    void createFramebuffers(
        VulkanContext &context,
        VulkanSwapchain &swapchain
    );

    void createCommandPool(
        VulkanContext &context
    );

    void createCommandBuffers(
        VulkanContext &context,
        VulkanSwapchain &swapchain
    );

    void recordCommandBuffer(
        VkCommandBuffer commandBuffer,
        uint32_t imageIndex,
        VkExtent2D extent
    );

    void createSyncObjects(
        VulkanContext& context,
        VulkanSwapchain& swapchain
    );

    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_framebuffers;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_commandBuffers;


    VkSemaphore m_imageAvailableSemaphore =
        VK_NULL_HANDLE;

    std::vector<VkSemaphore> m_renderFinishedSemaphores;

    VkFence m_inFlightFence =
        VK_NULL_HANDLE;
};

#endif //VKHORIZON_VULKANRENDERER_H
