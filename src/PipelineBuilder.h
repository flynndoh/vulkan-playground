#pragma once

#include "vulkan/vulkan.h"

#include <vector>

namespace vulkan_engine
{
class PipelineBuilder
{
  public:
    VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);

    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
    VkPipelineVertexInputStateCreateInfo vertex_input_info;
    VkViewport viewport;
    VkRect2D scissor;
    VkPipelineRasterizationStateCreateInfo rasteriser;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineInputAssemblyStateCreateInfo input_assembly;
    VkPipelineColorBlendAttachmentState colour_blend_attachment;
    VkPipelineLayout pipeline_layout;
};
} // namespace vulkan_engine
