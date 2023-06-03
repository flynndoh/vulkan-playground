#ifndef CPP_VULKAN_PIPELINEBUILDER_H
#define CPP_VULKAN_PIPELINEBUILDER_H


#include <vulkan/vulkan.h>
#include <vector>

class PipelineBuilder {

public:
    VkPipeline BuildPipeline(VkDevice device, VkRenderPass pass);

public:
    std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;
    VkPipelineVertexInputStateCreateInfo VertexInputInfo;
    VkViewport Viewport;
    VkRect2D Scissor;
    VkPipelineRasterizationStateCreateInfo Rasteriser;
    VkPipelineMultisampleStateCreateInfo Multisampling;
    VkPipelineInputAssemblyStateCreateInfo InputAssembly;
    VkPipelineColorBlendAttachmentState ColourBlendAttachment;
    VkPipelineLayout PipelineLayout;

};


#endif //CPP_VULKAN_PIPELINEBUILDER_H
