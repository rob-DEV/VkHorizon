#include "VulkanRenderer.h"

#include "VulkanContext.h"
#include "VulkanSwapchain.h"

#include <stdexcept>
#include <fstream>

void VulkanRenderer::init(
    VulkanContext &context,
    VulkanSwapchain &swapchain) {
    createRenderPass(context, swapchain);

    createGraphicsPipeline(context,swapchain);

    createFramebuffers(context, swapchain);
    createCommandPool(context);
    createCommandBuffers(context, swapchain);
    createSyncObjects(context, swapchain);
}

void VulkanRenderer::cleanup(
    VulkanContext &context) {

    vkDeviceWaitIdle(context.device());


    // ---------------------------------------------------------
    // Sync objects
    // ---------------------------------------------------------

    if (m_inFlightFence != VK_NULL_HANDLE) {
        vkDestroyFence(
            context.device(),
            m_inFlightFence,
            nullptr
        );
        m_inFlightFence = VK_NULL_HANDLE;
    }

    for (VkSemaphore semaphore : m_renderFinishedSemaphores) {
        vkDestroySemaphore(
            context.device(),
            semaphore,
            nullptr
        );
    }
    m_renderFinishedSemaphores.clear();

    if (m_imageAvailableSemaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(
            context.device(),
            m_imageAvailableSemaphore,
            nullptr
        );
        m_imageAvailableSemaphore = VK_NULL_HANDLE;
    }

    // ---------------------------------------------------------
    // Command pool (also frees the command buffers allocated from it)
    // ---------------------------------------------------------

    if (m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(
            context.device(),
            m_commandPool,
            nullptr
        );
        m_commandPool = VK_NULL_HANDLE;
    }
    m_commandBuffers.clear();

    // ---------------------------------------------------------
    // Framebuffers
    // ---------------------------------------------------------

    for (VkFramebuffer framebuffer: m_framebuffers) {
        vkDestroyFramebuffer(
            context.device(),
            framebuffer,
            nullptr
        );
    }
    m_framebuffers.clear();

    // ---------------------------------------------------------
    // Pipeline + layout
    // ---------------------------------------------------------

    if (m_graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(
            context.device(),
            m_graphicsPipeline,
            nullptr
        );
        m_graphicsPipeline = VK_NULL_HANDLE;
    }

    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(
            context.device(),
            m_pipelineLayout,
            nullptr
        );
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    // ---------------------------------------------------------
    // Render pass
    // ---------------------------------------------------------

    if (m_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(
            context.device(),
            m_renderPass,
            nullptr
        );
        m_renderPass = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::createRenderPass(
    VulkanContext &context,
    VulkanSwapchain &swapchain) {
    VkAttachmentDescription colorAttachment{};

    colorAttachment.format =
            swapchain.imageFormat();

    colorAttachment.samples =
            VK_SAMPLE_COUNT_1_BIT;

    colorAttachment.loadOp =
            VK_ATTACHMENT_LOAD_OP_CLEAR;

    colorAttachment.storeOp =
            VK_ATTACHMENT_STORE_OP_STORE;

    colorAttachment.stencilLoadOp =
            VK_ATTACHMENT_LOAD_OP_DONT_CARE;

    colorAttachment.stencilStoreOp =
            VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout =
            VK_IMAGE_LAYOUT_UNDEFINED;

    colorAttachment.finalLayout =
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};

    colorAttachmentRef.attachment = 0;

    colorAttachmentRef.layout =
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};

    subpass.pipelineBindPoint =
            VK_PIPELINE_BIND_POINT_GRAPHICS;

    subpass.colorAttachmentCount = 1;

    subpass.pColorAttachments =
            &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};

    renderPassInfo.sType =
            VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

    renderPassInfo.attachmentCount = 1;

    renderPassInfo.pAttachments =
            &colorAttachment;

    renderPassInfo.subpassCount = 1;

    renderPassInfo.pSubpasses =
            &subpass;

    if (vkCreateRenderPass(
            context.device(),
            &renderPassInfo,
            nullptr,
            &m_renderPass) != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create render pass"
        );
    }
}

static std::vector<char> readFile(
    const std::string &filename) {
    std::ifstream file(
        filename,
        std::ios::ate |
        std::ios::binary
    );

    if (!file) {
        throw std::runtime_error(
            "Failed to open shader file: " +
            filename
        );
    }

    size_t fileSize =
            static_cast<size_t>(
                file.tellg()
            );

    std::vector<char> buffer(fileSize);

    file.seekg(0);

    file.read(
        buffer.data(),
        fileSize
    );

    return buffer;
}

VkShaderModule createShaderModule(
    VkDevice device,
    const std::vector<char> &code) {
    VkShaderModuleCreateInfo createInfo{};

    createInfo.sType =
            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    createInfo.codeSize =
            code.size();

    createInfo.pCode =
            reinterpret_cast<const uint32_t *>(
                code.data()
            );

    VkShaderModule shaderModule;

    if (vkCreateShaderModule(
            device,
            &createInfo,
            nullptr,
            &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create shader module"
        );
    }

    return shaderModule;
}


void VulkanRenderer::createGraphicsPipeline(VulkanContext &context, VulkanSwapchain &swapchain) {
    auto vertShaderCode =
            readFile(
                "../shaders/bin/triangle.vert.spv"
            );

    auto fragShaderCode =
            readFile(
                "../shaders/bin/triangle.frag.spv"
            );

    VkShaderModule vertShaderModule =
            createShaderModule(
                context.device(),
                vertShaderCode
            );

    VkShaderModule fragShaderModule =
            createShaderModule(
                context.device(),
                fragShaderCode
            );

    VkPipelineShaderStageCreateInfo vertStage{};

    vertStage.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

    vertStage.stage =
            VK_SHADER_STAGE_VERTEX_BIT;

    vertStage.module =
            vertShaderModule;

    vertStage.pName =
            "main";

    VkPipelineShaderStageCreateInfo fragStage{};

    fragStage.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

    fragStage.stage =
            VK_SHADER_STAGE_FRAGMENT_BIT;

    fragStage.module =
            fragShaderModule;

    fragStage.pName =
            "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertStage,
        fragStage
    };

    VkPipelineVertexInputStateCreateInfo vertexInput{};

    vertexInput.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertexInput.vertexBindingDescriptionCount = 0;

    vertexInput.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};

    inputAssembly.sType =
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

    inputAssembly.topology =
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    inputAssembly.primitiveRestartEnable =
            VK_FALSE;

    VkViewport viewport{};

    viewport.x = 0.0f;
    viewport.y = 0.0f;

    viewport.width =
            static_cast<float>(
                swapchain.extent().width
            );

    viewport.height =
            static_cast<float>(
                swapchain.extent().height
            );

    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};

    scissor.offset = {
        0,
        0
    };

    scissor.extent =
            swapchain.extent();

    VkPipelineViewportStateCreateInfo viewportState{};

    viewportState.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

    viewportState.viewportCount = 1;

    viewportState.pViewports =
            &viewport;

    viewportState.scissorCount = 1;

    viewportState.pScissors =
            &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};

    rasterizer.sType =
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

    rasterizer.depthClampEnable =
            VK_FALSE;

    rasterizer.rasterizerDiscardEnable =
            VK_FALSE;

    rasterizer.polygonMode =
            VK_POLYGON_MODE_FILL;

    rasterizer.lineWidth =
            1.0f;

    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;

    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    rasterizer.depthBiasEnable =
            VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};

    multisampling.sType =
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

    multisampling.sampleShadingEnable =
            VK_FALSE;

    multisampling.rasterizationSamples =
            VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};

    colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;

    colorBlendAttachment.blendEnable =
            VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};

    colorBlending.sType =
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

    colorBlending.logicOpEnable =
            VK_FALSE;

    colorBlending.logicOp =
            VK_LOGIC_OP_COPY;

    colorBlending.attachmentCount =
            1;

    colorBlending.pAttachments =
            &colorBlendAttachment;

    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};

    pipelineLayoutInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    pipelineLayoutInfo.setLayoutCount = 0;

    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(
            context.device(),
            &pipelineLayoutInfo,
            nullptr,
            &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create pipeline layout"
        );
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};

    pipelineInfo.sType =
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    pipelineInfo.stageCount = 2;

    pipelineInfo.pStages =
            shaderStages;

    pipelineInfo.pVertexInputState =
            &vertexInput;

    pipelineInfo.pInputAssemblyState =
            &inputAssembly;

    pipelineInfo.pViewportState =
            &viewportState;

    pipelineInfo.pRasterizationState =
            &rasterizer;

    pipelineInfo.pMultisampleState =
            &multisampling;

    pipelineInfo.pColorBlendState =
            &colorBlending;

    pipelineInfo.layout =
            m_pipelineLayout;

    pipelineInfo.renderPass =
            m_renderPass;

    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(
            context.device(),
            VK_NULL_HANDLE,
            1,
            &pipelineInfo,
            nullptr,
            &m_graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create graphics pipeline"
        );
    }

    vkDestroyShaderModule(
        context.device(),
        fragShaderModule,
        nullptr
    );

    vkDestroyShaderModule(
        context.device(),
        vertShaderModule,
        nullptr
    );
}

void VulkanRenderer::createFramebuffers(
    VulkanContext &context,
    VulkanSwapchain &swapchain) {
    m_framebuffers.resize(
        swapchain.imageViews().size()
    );

    for (size_t i = 0;
         i < swapchain.imageViews().size();
         ++i) {
        VkImageView attachments[] = {
            swapchain.imageViews()[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};

        framebufferInfo.sType =
                VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;

        framebufferInfo.renderPass =
                m_renderPass;

        framebufferInfo.attachmentCount =
                1;

        framebufferInfo.pAttachments =
                attachments;

        framebufferInfo.width =
                swapchain.extent().width;

        framebufferInfo.height =
                swapchain.extent().height;

        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(
                context.device(),
                &framebufferInfo,
                nullptr,
                &m_framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to create framebuffer"
            );
        }
    }
}

void VulkanRenderer::createCommandPool(
    VulkanContext &context) {
    VkCommandPoolCreateInfo poolInfo{};

    poolInfo.sType =
            VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

    poolInfo.flags =
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    poolInfo.queueFamilyIndex =
            context.graphicsQueueFamily();

    if (vkCreateCommandPool(
            context.device(),
            &poolInfo,
            nullptr,
            &m_commandPool) != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create command pool"
        );
    }
}

void VulkanRenderer::createCommandBuffers(
    VulkanContext &context,
    VulkanSwapchain &swapchain) {
    m_commandBuffers.resize(
        m_framebuffers.size()
    );

    VkCommandBufferAllocateInfo allocInfo{};

    allocInfo.sType =
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

    allocInfo.commandPool =
            m_commandPool;

    allocInfo.level =
            VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    allocInfo.commandBufferCount =
            static_cast<uint32_t>(
                m_commandBuffers.size()
            );

    if (vkAllocateCommandBuffers(
            context.device(),
            &allocInfo,
            m_commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to allocate command buffers"
        );
    }

    // Record each command buffer
    for (size_t i = 0;
         i < m_commandBuffers.size();
         ++i) {
        recordCommandBuffer(
            m_commandBuffers[i],
            static_cast<uint32_t>(i),
            swapchain.extent()
        );
    }
}

void VulkanRenderer::recordCommandBuffer(
    VkCommandBuffer commandBuffer,
    uint32_t imageIndex,
    VkExtent2D extent) {
    VkCommandBufferBeginInfo beginInfo{};

    beginInfo.sType =
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(
            commandBuffer,
            &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to begin recording command buffer"
        );
    }
    VkClearValue clearColor{};

    clearColor.color = {
        0.02f,
        0.02f,
        0.03f,
        1.0f
    };

    VkRenderPassBeginInfo renderPassInfo{};

    renderPassInfo.sType =
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

    renderPassInfo.renderPass =
            m_renderPass;

    renderPassInfo.framebuffer =
            m_framebuffers[imageIndex];

    renderPassInfo.renderArea.offset = {
        0,
        0
    };

    renderPassInfo.renderArea.extent = extent;

    renderPassInfo.clearValueCount = 1;

    renderPassInfo.pClearValues =
            &clearColor;

    vkCmdBeginRenderPass(
        commandBuffer,
        &renderPassInfo,
        VK_SUBPASS_CONTENTS_INLINE
    );

    vkCmdBindPipeline(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_graphicsPipeline
    );

    vkCmdDraw(
        commandBuffer,
        3, // vertexCount
        1, // instanceCount
        0, // firstVertex
        0 // firstInstance
    );

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to record command buffer"
        );
    }
}

void VulkanRenderer::createSyncObjects(
    VulkanContext &context,
    VulkanSwapchain &swapchain) {
    VkSemaphoreCreateInfo semaphoreInfo{};

    semaphoreInfo.sType =
            VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};

    fenceInfo.sType =
            VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    fenceInfo.flags =
            VK_FENCE_CREATE_SIGNALED_BIT;


    if (vkCreateSemaphore(
            context.device(),
            &semaphoreInfo,
            nullptr,
            &m_imageAvailableSemaphore) != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create image available semaphore"
        );
    }


    m_renderFinishedSemaphores.resize(
        swapchain.imageViews().size()
    );

    for (auto &semaphore: m_renderFinishedSemaphores) {
        if (vkCreateSemaphore(
                context.device(),
                &semaphoreInfo,
                nullptr,
                &semaphore) != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to create render finished semaphore"
            );
        }
    }


    if (vkCreateFence(
            context.device(),
            &fenceInfo,
            nullptr,
            &m_inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create in-flight fence"
        );
    }
}

void VulkanRenderer::drawFrame(
    VulkanContext &context,
    VulkanSwapchain &swapchain) {
    // ---------------------------------------------------------
    // Wait for the previous frame to finish
    // ---------------------------------------------------------

    if (vkWaitForFences(
            context.device(),
            1,
            &m_inFlightFence,
            VK_TRUE,
            UINT64_MAX) != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed waiting for in-flight fence"
        );
    }

    // Fence is now signaled, so we can reuse it.
    vkResetFences(
        context.device(),
        1,
        &m_inFlightFence
    );


    // ---------------------------------------------------------
    // Acquire the next swapchain image
    // ---------------------------------------------------------

    uint32_t imageIndex;

    VkResult result = vkAcquireNextImageKHR(
        context.device(),
        swapchain.handle(),
        UINT64_MAX,
        m_imageAvailableSemaphore,
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // We'll handle swapchain recreation later.
        return;
    }

    if (result != VK_SUCCESS &&
        result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error(
            "Failed to acquire swapchain image"
        );
    }


    // ---------------------------------------------------------
    // Select the semaphore associated with this image
    // ---------------------------------------------------------

    VkSemaphore renderFinishedSemaphore =
            m_renderFinishedSemaphores[imageIndex];


    // ---------------------------------------------------------
    // Submit command buffer
    // ---------------------------------------------------------

    VkSemaphore waitSemaphores[] = {
        m_imageAvailableSemaphore
    };

    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSemaphore signalSemaphores[] = {
        renderFinishedSemaphore
    };


    VkSubmitInfo submitInfo{};

    submitInfo.sType =
            VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // Wait for the image to become available
    submitInfo.waitSemaphoreCount = 1;

    submitInfo.pWaitSemaphores =
            waitSemaphores;

    submitInfo.pWaitDstStageMask =
            waitStages;

    // Command buffer to execute
    submitInfo.commandBufferCount = 1;

    submitInfo.pCommandBuffers =
            &m_commandBuffers[imageIndex];

    // Signal when rendering is complete
    submitInfo.signalSemaphoreCount = 1;

    submitInfo.pSignalSemaphores =
            signalSemaphores;


    if (vkQueueSubmit(
            context.graphicsQueue(),
            1,
            &submitInfo,
            m_inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to submit draw command buffer"
        );
    }


    // ---------------------------------------------------------
    // Present the rendered image
    // ---------------------------------------------------------

    VkSwapchainKHR swapchains[] = {
        swapchain.handle()
    };

    VkPresentInfoKHR presentInfo{};

    presentInfo.sType =
            VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    // Wait until rendering has finished
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pWaitSemaphores =
            signalSemaphores;

    // Swapchain to present to
    presentInfo.swapchainCount = 1;

    presentInfo.pSwapchains =
            swapchains;

    // Which swapchain image
    presentInfo.pImageIndices =
            &imageIndex;


    result = vkQueuePresentKHR(
        context.presentQueue(),
        &presentInfo
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR ||
        result == VK_SUBOPTIMAL_KHR) {
        // We'll handle swapchain recreation later.
        return;
    }

    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to present swapchain image"
        );
    }
}
