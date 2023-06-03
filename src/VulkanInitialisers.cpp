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