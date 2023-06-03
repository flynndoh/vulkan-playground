#include <VulkanInitialisers.h>

VkCommandPoolCreateInfo
VulkanInitialisers::CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags /*= 0*/)
{
    VkCommandPoolCreateInfo commandPoolInfo = {}; // initialise entire struct to 0's
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.pNext = nullptr;

    // command pool will be one that can submit graphics commands
    commandPoolInfo.queueFamilyIndex = queueFamilyIndex;

    // we also want the pool to allow for the resetting of individual command buffers
    commandPoolInfo.flags = flags;
    return commandPoolInfo;
}

VkCommandBufferAllocateInfo VulkanInitialisers::CommandBufferAllocateInfo(VkCommandPool pool, uint32_t count /*= 1*/,
                                                                          VkCommandBufferLevel level /*= VK_COMMAND_BUFFER_LEVEL_PRIMARY*/)
{
    VkCommandBufferAllocateInfo commandAllocInfo = {}; // initialise entire struct to 0's
    commandAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandAllocInfo.pNext = nullptr;

    // commands will be made from our _commandPool
    commandAllocInfo.commandPool = pool;

    // allocate 1 command buffer
    commandAllocInfo.commandBufferCount = count;

    // command level is primary
    commandAllocInfo.level = level;
    return commandAllocInfo;
}

VkPipelineShaderStageCreateInfo
VulkanInitialisers::PipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule)
{
    VkPipelineShaderStageCreateInfo shaderStageInfo = {}; // initialise entire struct to 0's
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.pNext = nullptr;
    shaderStageInfo.stage = stage; // shader stage
    shaderStageInfo.module = shaderModule; // module containing the code for this shader stage
    shaderStageInfo.pName = "main"; // the entry point of the shader
    return shaderStageInfo;
}

VkPipelineVertexInputStateCreateInfo VulkanInitialisers::VertexInputStateCreateInfo()
{
    VkPipelineVertexInputStateCreateInfo vertexInputStateInfo = {}; // initialise entire struct to 0's
    vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateInfo.pNext = nullptr;

    // no vertex bindings or attributes
    vertexInputStateInfo.vertexBindingDescriptionCount = 0;
    vertexInputStateInfo.vertexAttributeDescriptionCount = 0;
    return vertexInputStateInfo;
}

VkPipelineInputAssemblyStateCreateInfo VulkanInitialisers::InputAssemblyCreateInfo(VkPrimitiveTopology topology)
{
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo = {}; // initialise entire struct to 0's
    inputAssemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateInfo.pNext = nullptr;

    inputAssemblyStateInfo.topology = topology;
    inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;
    return inputAssemblyStateInfo;
}

VkPipelineRasterizationStateCreateInfo VulkanInitialisers::RasterisationStateCreateInfo(VkPolygonMode polygonMode)
{
    VkPipelineRasterizationStateCreateInfo rasterisationStateInfo = {}; // initialise entire struct to 0's
    rasterisationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterisationStateInfo.pNext = nullptr;

    rasterisationStateInfo.depthClampEnable = VK_FALSE;

    // if this is enabled, it will discard all primitives before the rasterisation stage which we don't want
    rasterisationStateInfo.rasterizerDiscardEnable = VK_FALSE;

    // polygon mode can be used to change between wireframe and solid drawing
    rasterisationStateInfo.polygonMode = polygonMode;
    rasterisationStateInfo.lineWidth = 1.0f;

    // disable backface culling
    rasterisationStateInfo.cullMode = VK_CULL_MODE_NONE;
    rasterisationStateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

    // no depth bias
    rasterisationStateInfo.depthBiasEnable = VK_FALSE;
    rasterisationStateInfo.depthBiasConstantFactor = 0.0f;
    rasterisationStateInfo.depthBiasClamp = 0.0f;
    rasterisationStateInfo.depthBiasSlopeFactor = 0.0f;
    return rasterisationStateInfo;
}

VkPipelineMultisampleStateCreateInfo VulkanInitialisers::MultisamplingStateCreateInfo()
{
    VkPipelineMultisampleStateCreateInfo multisampleStateInfo = {}; // initialise entire struct to 0's
    multisampleStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateInfo.pNext = nullptr;

    multisampleStateInfo.sampleShadingEnable = VK_FALSE;

    // multisampling disabled by default (1 sample per pixel)
    multisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleStateInfo.minSampleShading = 1.0f;
    multisampleStateInfo.pSampleMask = nullptr;
    multisampleStateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleStateInfo.alphaToOneEnable = VK_FALSE;
    return multisampleStateInfo;
}

VkPipelineColorBlendAttachmentState VulkanInitialisers::ColorBlendAttachmentState()
{
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {}; // initialise entire struct to 0's
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;;
    colorBlendAttachment.blendEnable = VK_FALSE; // no blending
    return colorBlendAttachment;
}

VkPipelineLayoutCreateInfo VulkanInitialisers::PipelineLayoutCreateInfo()
{
    VkPipelineLayoutCreateInfo layoutInfo = {}; // initialise struct to 0's
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pNext = nullptr;

    // just some empty defaults
    layoutInfo.flags = 0;
    layoutInfo.setLayoutCount = 0;
    layoutInfo.pSetLayouts = nullptr;
    layoutInfo.pushConstantRangeCount = 0; // shader currently has no inputs
    layoutInfo.pPushConstantRanges = nullptr; // shader currently has no inputs
    return layoutInfo;
}