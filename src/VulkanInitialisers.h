// cpp-vulkan.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <VulkanTypes.h>

namespace VulkanInitialisers {

    VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);

    VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1,
                                                          VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    VkPipelineShaderStageCreateInfo
    PipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule);

    VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo();

    VkPipelineInputAssemblyStateCreateInfo InputAssemblyCreateInfo(VkPrimitiveTopology topology);

    VkPipelineRasterizationStateCreateInfo RasterisationStateCreateInfo(VkPolygonMode polygonMode);

    VkPipelineMultisampleStateCreateInfo MultisamplingStateCreateInfo();

    VkPipelineColorBlendAttachmentState ColorBlendAttachmentState();

    VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo();
}

