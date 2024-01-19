#include "VulkanInitialisers.h"

namespace vulkan_engine::initialisers
{
VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags /*= 0*/)
{
    VkCommandPoolCreateInfo command_pool_info = {}; // initialise entire struct to 0's
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.pNext = nullptr;

    // command pool will be one that can submit graphics commands
    command_pool_info.queueFamilyIndex = queueFamilyIndex;

    // we also want the pool to allow for the resetting of individual command buffers
    command_pool_info.flags = flags;
    return command_pool_info;
}

VkCommandBufferAllocateInfo command_buffer_allocate_info(
    VkCommandPool pool, uint32_t count /*= 1*/, VkCommandBufferLevel level /*= VK_COMMAND_BUFFER_LEVEL_PRIMARY*/)
{
    VkCommandBufferAllocateInfo command_alloc_info = {}; // initialise entire struct to 0's
    command_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_alloc_info.pNext = nullptr;

    // commands will be made from our _command_pool
    command_alloc_info.commandPool = pool;

    // allocate 1 command buffer
    command_alloc_info.commandBufferCount = count;

    // command level is primary
    command_alloc_info.level = level;
    return command_alloc_info;
}

VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage,
                                                                  VkShaderModule shaderModule)
{
    VkPipelineShaderStageCreateInfo shader_stage_info = {}; // initialise entire struct to 0's
    shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_info.pNext = nullptr;
    shader_stage_info.stage = stage;         // shader stage
    shader_stage_info.module = shaderModule; // module containing the code for this shader stage
    shader_stage_info.pName = "main";        // the entry point of the shader
    return shader_stage_info;
}

VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info()
{
    VkPipelineVertexInputStateCreateInfo vertex_input_state_info = {}; // initialise entire struct to 0's
    vertex_input_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_info.pNext = nullptr;

    // no vertex bindings or attributes
    vertex_input_state_info.vertexBindingDescriptionCount = 0;
    vertex_input_state_info.vertexAttributeDescriptionCount = 0;
    return vertex_input_state_info;
}

VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info(VkPrimitiveTopology topology)
{
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_info = {}; // initialise entire struct to 0's
    input_assembly_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_info.pNext = nullptr;

    input_assembly_state_info.topology = topology;
    input_assembly_state_info.primitiveRestartEnable = VK_FALSE;
    return input_assembly_state_info;
}

VkPipelineRasterizationStateCreateInfo rasterisation_state_create_info(VkPolygonMode polygonMode)
{
    VkPipelineRasterizationStateCreateInfo rasterisation_state_info = {}; // initialise entire struct to 0's
    rasterisation_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterisation_state_info.pNext = nullptr;

    rasterisation_state_info.depthClampEnable = VK_FALSE;

    // if this is enabled, it will discard all primitives before the rasterisation
    // stage which we don't want
    rasterisation_state_info.rasterizerDiscardEnable = VK_FALSE;

    // polygon mode can be used to change between wireframe and solid drawing
    rasterisation_state_info.polygonMode = polygonMode;
    rasterisation_state_info.lineWidth = 1.0f;

    // disable backface culling
    rasterisation_state_info.cullMode = VK_CULL_MODE_NONE;
    rasterisation_state_info.frontFace = VK_FRONT_FACE_CLOCKWISE;

    // no depth bias
    rasterisation_state_info.depthBiasEnable = VK_FALSE;
    rasterisation_state_info.depthBiasConstantFactor = 0.0f;
    rasterisation_state_info.depthBiasClamp = 0.0f;
    rasterisation_state_info.depthBiasSlopeFactor = 0.0f;
    return rasterisation_state_info;
}

VkPipelineMultisampleStateCreateInfo multisampling_state_create_info()
{
    VkPipelineMultisampleStateCreateInfo multisample_state_info = {}; // initialise entire struct to 0's
    multisample_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_info.pNext = nullptr;

    multisample_state_info.sampleShadingEnable = VK_FALSE;

    // multisampling disabled by default (1 sample per pixel)
    multisample_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_info.minSampleShading = 1.0f;
    multisample_state_info.pSampleMask = nullptr;
    multisample_state_info.alphaToCoverageEnable = VK_FALSE;
    multisample_state_info.alphaToOneEnable = VK_FALSE;
    return multisample_state_info;
}

VkPipelineColorBlendAttachmentState color_blend_attachment_state()
{
    VkPipelineColorBlendAttachmentState color_blend_attachment = {}; // initialise entire struct to 0's
    color_blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    color_blend_attachment.blendEnable = VK_FALSE; // no blending
    return color_blend_attachment;
}

VkPipelineLayoutCreateInfo pipeline_layout_create_info()
{
    VkPipelineLayoutCreateInfo layout_info = {}; // initialise struct to 0's
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.pNext = nullptr;

    // just some empty defaults
    layout_info.flags = 0;
    layout_info.setLayoutCount = 0;
    layout_info.pSetLayouts = nullptr;
    layout_info.pushConstantRangeCount = 0;    // shader currently has no inputs
    layout_info.pPushConstantRanges = nullptr; // shader currently has no inputs
    return layout_info;
}

} // namespace vulkan_engine::initialisers