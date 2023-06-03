#include <iostream>
#include "PipelineBuilder.h"

VkPipeline PipelineBuilder::BuildPipeline(VkDevice device, VkRenderPass pass)
{
    // make Viewport state from our stored Viewport and scissor
    // currently doesn't support multiple viewports or scissors
    VkPipelineViewportStateCreateInfo viewportState = {}; // initialise struct to 0's
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;

    viewportState.viewportCount = 1;
    viewportState.pViewports = &Viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &Scissor;

    // setup some dummy colour blending. Not using transparent objects yet though
    VkPipelineColorBlendStateCreateInfo colourBlending = {}; // initialise struct to 0's
    colourBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colourBlending.pNext = nullptr;

    colourBlending.logicOpEnable = VK_FALSE;
    colourBlending.logicOp = VK_LOGIC_OP_COPY;
    colourBlending.attachmentCount = 1;
    colourBlending.pAttachments = &ColourBlendAttachment;

    // build the pipeline config
    VkGraphicsPipelineCreateInfo pipelineInfo = {}; // initialise struct to 0's
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;

    pipelineInfo.stageCount = ShaderStages.size();
    pipelineInfo.pStages = ShaderStages.data();
    pipelineInfo.pVertexInputState = &VertexInputInfo;
    pipelineInfo.pInputAssemblyState = &InputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &Rasteriser;
    pipelineInfo.pMultisampleState = &Multisampling;
    pipelineInfo.pColorBlendState = &colourBlending;
    pipelineInfo.layout = PipelineLayout;
    pipelineInfo.renderPass = pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    // create the pipeline object and handle any errors
    VkPipeline newPipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS)
    {
        std::cout << "Failed to create graphics pipeline" << std::endl;
        return VK_NULL_HANDLE;
    }

    return newPipeline;
}

















