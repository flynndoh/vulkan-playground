#include <cmath>
#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <VkBootstrap.h>
#include <fstream>

#include "VulkanEngine.h"
#include "VulkanInitialisers.h"
#include "PipelineBuilder.h"

// We want to immediately abort when there is an error. In normal engines this would give an error message to the user,
// or perform a dump of state.
#define VK_CHECK(x)                                                         \
    do                                                                      \
    {                                                                       \
        VkResult err = x;                                                   \
        if (err)                                                            \
        {                                                                   \
            std::cout << "Detected Vulkan Error: " << err << std::endl;     \
            abort();                                                        \
        }                                                                   \
    } while (0)                                                             \


void VulkanEngine::Init()
{
    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

    auto window_flags = (SDL_WindowFlags) (SDL_WINDOW_VULKAN);

    _window = SDL_CreateWindow(
            "rendering window",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            _windowExtent.width,
            _windowExtent.height,
            window_flags
    );

    // load the core Vulkan structures
    InitVulcan();

    // create the swap chain (image buffers to be used when frame buffering)
    InitSwapchain();

    // create the queue to facilitate sending commands to the GPU
    InitCommands();

    InitDefaultRenderPass();

    InitFramebuffers();

    // create fences to sync comms from the GPU to the CPU. Create semaphores to sync comms between GPU and GPU
    InitSyncStructures();

    InitPipelines();

    // everything went fine
    _isInitialized = true;
}

void VulkanEngine::Cleanup() const
{
    // NOTE: We must destroy objects in the reverse order in which they were created
    if (_isInitialized)
    {
        // make sure the GPU has stopped doing its things
        vkDeviceWaitIdle(_device);

        // destroy command pool
        vkDestroyCommandPool(_device, _commandPool, nullptr);

        // destroy sync objects
        vkDestroyFence(_device, _renderFence, nullptr);
        vkDestroySemaphore(_device, _renderSemaphore, nullptr);
        vkDestroySemaphore(_device, _presentSemaphore, nullptr);

        // destroy swapchain
        vkDestroySwapchainKHR(_device, _swapchain, nullptr);

        // destroy the main render pass
        vkDestroyRenderPass(_device, _renderPass, nullptr);

        // destroy swapchain resources
        for (int i = 0; i < _swapchainImageViews.size(); ++i)
        {
            vkDestroyFramebuffer(_device, _framebuffers[i], nullptr);

            vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
        }

        vkDestroyDevice(_device, nullptr);
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkb::destroy_debug_utils_messenger(_instance, _debugMessenger);
        vkDestroyInstance(_instance, nullptr);

        SDL_DestroyWindow(_window);
    }
}

void VulkanEngine::Draw()
{
    // wait until the GPU has finished rendering the previous frame, with a timeout of 1 second
    VK_CHECK(vkWaitForFences(_device, 1, &_renderFence, true, 1000000000/*ns*/));
    VK_CHECK(vkResetFences(_device, 1, &_renderFence));

    // request an image from the swapchain, with a timeout of 1 second
    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000/*ns*/, _presentSemaphore, nullptr,
                                   &swapchainImageIndex));

    // at this point, we are sure that the commands have finished executing, and we can safely reset the command buffer
    // before we begin recording to it again
    VK_CHECK(vkResetCommandBuffer(_mainCommandBuffer, 0));

    VkCommandBuffer commandBuffer = _mainCommandBuffer;

    // begin the command buffer recording. We will use this command buffer exactly one time, so we want to let Vulkan
    // know that
    VkCommandBufferBeginInfo commandBufferBeginInfo = {}; // initialise structure to 0's
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = nullptr;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

    // make a clear colour from frame number. This will flash with a 120*pi frame period
    VkClearValue clearValue;
    float flash = abs(std::sin(_frameNumber / 120.f));
    clearValue.color = {{0.0f, 0.0f, flash, 1.0f}};

    // start with the main render pass
    // we will use the clear colour defined above, and the framebuffer of the index the swapchain gave us
    VkRenderPassBeginInfo renderPassBeginInfo = {}; // initialise struct with 0's
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = _renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent = _windowExtent;
    renderPassBeginInfo.framebuffer = _framebuffers[swapchainImageIndex]; // this is where we render into

    // connect up clear values
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearValue;

    // begin this render pass
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _trianglePipeline);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    // finalise this render pass
    vkCmdEndRenderPass(commandBuffer);

    // finalise command buffer (we can no longer add commands, but it can now be executed by the GPU)
    VK_CHECK(vkEndCommandBuffer(commandBuffer));

    // prepare submision to the queue
    // we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
    // we will signal the _renderSemaphore to signal that rendering has finished
    VkSubmitInfo submit = {}; // initialise struct to 0's
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.pNext = nullptr;

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit.pWaitDstStageMask = &waitStage;

    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &_presentSemaphore;

    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &_renderSemaphore;

    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commandBuffer;

    // submit the command bugger to the queue and execute it
    // _renderFence will now block until the GPU finishes executing the graphics commands
    VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, _renderFence));

    // this will put the image we just rendered into the visible window
    // we want to wait on the _renderSemaphore for that as it's necessary that all drawing commands have finished before
    // the image is displayed to the user
    VkPresentInfoKHR presentInfo = {}; // initialise struct to 0's
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapchain;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &_renderSemaphore;

    presentInfo.pImageIndices = &swapchainImageIndex;

    // present to user's screen
    VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

    // increment the number of frames drawn
    _frameNumber++;
}

void VulkanEngine::Run()
{
    SDL_Event e;
    bool bQuit = false;

    // main loop
    while (!bQuit)
    {
        //Handle events on queue
        while (SDL_PollEvent(&e) != 0)
        {
            // close the window when user alt-f4s or clicks the X button
            if (e.type == SDL_QUIT) { bQuit = true; }
        }

        Draw();
    }
}

void VulkanEngine::InitVulcan()
{
    vkb::InstanceBuilder builder;

    auto instance = builder.set_app_name("Vulkan Playground")
            .request_validation_layers(true) // TODO: In prod, we would remove this to improve performance
            .require_api_version(1, 1, 0)
            .use_default_debug_messenger() // Use this to catch validation errors
            .build();

    vkb::Instance vkbInstance = instance.value();

    // persist the instance for reuse
    _instance = vkbInstance.instance;

    // also persist debug messenger
    _debugMessenger = vkbInstance.debug_messenger;

    // create a surface using the window we opened with SDL
    SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

    // select a physical GPU to use, using the vk bootstrap library to choose for us
    vkb::PhysicalDeviceSelector selector{vkbInstance};
    vkb::PhysicalDevice physicalDevice = selector
            .set_minimum_version(1, 1)
            .set_surface(_surface)
            .select()
            .value();

    // create the logical Vulkan device using the selected physical GPU
    vkb::DeviceBuilder deviceBuilder{physicalDevice};
    vkb::Device vkbDevice = deviceBuilder.build().value();

    // persist for later usage
    _device = vkbDevice.device;
    _chosenGPU = physicalDevice.physical_device;

    // use bootstrapper to get a graphics queue
    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
}

void VulkanEngine::InitSwapchain()
{
    vkb::SwapchainBuilder swapchainBuilder{_chosenGPU, _device, _surface};

    vkb::Swapchain vkbSwapchain = swapchainBuilder
            .use_default_format_selection()
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // TODO: vsync present mode
            .set_desired_extent(_windowExtent.width,
                                _windowExtent.height) // TODO: Need to rebuild the swapchain whenever the window is resized
            .build()
            .value();

    // persist swapchain and related image stuff
    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapchainImageViews = vkbSwapchain.get_image_views().value();
    _swapchainImageFormat = vkbSwapchain.image_format;
}

void VulkanEngine::InitCommands()
{
    // create command pool for commands submitted to the graphics queue
    // we also want the pool to allow for resetting of individual command buffers
    VkCommandPoolCreateInfo commandPoolInfo = VulkanInitialisers::CommandPoolCreateInfo(_graphicsQueueFamily,
                                                                                        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_commandPool));

    // allocate the default command buffer that we will use for rendering
    VkCommandBufferAllocateInfo commandAllocInfo = VulkanInitialisers::CommandBufferAllocateInfo(_commandPool, 1,
                                                                                                 VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    VK_CHECK(vkAllocateCommandBuffers(_device, &commandAllocInfo, &_mainCommandBuffer));
}

void VulkanEngine::InitDefaultRenderPass()
{
    VkAttachmentDescription colourAttachment = CreateColourAttachment();
    VkSubpassDescription singleSubpass = CreateSubpass();

    VkRenderPassCreateInfo renderPassInfo = {}; // initialise struct to 0's
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

    // connect the colour attachment to the render pass info
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colourAttachment;

    // connect the single subpass to the render pass info
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &singleSubpass;

    VK_CHECK(vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_renderPass));
}

VkAttachmentDescription VulkanEngine::CreateColourAttachment()
{
    // the render pass will use this colour attachment
    VkAttachmentDescription colourAttachment = {};

    // same attachment will have the format needed by the swapchain
    colourAttachment.format = _swapchainImageFormat;

    // 1 sample, can do higher samples for MSAA
    colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    // clear when this attachment is loaded
    colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

    // we keep the attachment stored when the render pass ends
    colourAttachment.storeOp - VK_ATTACHMENT_STORE_OP_STORE;

    // dont care about stencil
    colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // we don't know, nor do we care, about the starting layout of the attachment
    colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // once the render pass ends, the image needs to be in a layout that is ready for display
    colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    return colourAttachment;
}

VkSubpassDescription VulkanEngine::CreateSubpass()
{
    VkAttachmentReference attachmentReference = {}; // initialise the struct with 0's

    // attachment number will index into the pAttachments array in the parent render pass itself
    attachmentReference.attachment = 0;
    attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // only want to create 1 subpass, which is the minimum number you cna do
    VkSubpassDescription subpass = {}; // initialise the struct with 0's
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachmentReference;

    return subpass;
}

void VulkanEngine::InitFramebuffers()
{
    // create the framebuffers for the swapchain images. This will connect the render pass to the images for rendering
    VkFramebufferCreateInfo frameBufferInfo = {}; // initialise struct with 0's
    frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferInfo.pNext = nullptr;

    frameBufferInfo.renderPass = _renderPass;
    frameBufferInfo.attachmentCount = 1;
    frameBufferInfo.width = _windowExtent.width;
    frameBufferInfo.height = _windowExtent.height;
    frameBufferInfo.layers = 1;

    // grab the number of images we have in the swapchain
    const uint32_t swapchainImageCount = _swapchainImages.size();
    _framebuffers = std::vector<VkFramebuffer>(swapchainImageCount);

    // create a framebuffer for each of the swapchain image views
    for (int i = 0; i < swapchainImageCount; ++i)
    {
        frameBufferInfo.pAttachments = &_swapchainImageViews[i];
        VK_CHECK(vkCreateFramebuffer(_device, &frameBufferInfo, nullptr, &_framebuffers[i]));
    }
}

void VulkanEngine::InitSyncStructures()
{
    // create sync structures
    VkFenceCreateInfo fenceInfo = {}; // initialise structure with 0's
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = nullptr;

    // create the GPU --> CPU fence with the "CREATE_SIGNALED" flag, so that we can wait on it before using it on a GPU
    // command (for the first frame)
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CHECK(vkCreateFence(_device, &fenceInfo, nullptr, &_renderFence));

    // for the semaphores, we don't need much setup
    VkSemaphoreCreateInfo semaphoreInfo = {}; // initialise structure with 0's
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreInfo.pNext = nullptr;
    semaphoreInfo.flags = 0;

    // create presentation semaphore
    VK_CHECK(vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_presentSemaphore));

    // create rendering semaphore
    VK_CHECK(vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_renderSemaphore));
}

void VulkanEngine::InitPipelines()
{
    VkShaderModule triangleFragmentShader; // TODO: this is currently leaked
    if (!LoadShaderModule("../shaders/triangle.frag.spv", &triangleFragmentShader))
    {
        std::cout << "Error when building the triangle fragment shader module" << std::endl;
    }
    else
    {
        std::cout << "Triangle fragment shader successfully loaded" << std::endl;
    }

    VkShaderModule triangleVertexShader; // TODO: this is currently leaked
    if (!LoadShaderModule("../shaders/triangle.vert.spv", &triangleVertexShader))
    {
        std::cout << "Error when building the triangle vertex shader module" << std::endl;
    }
    else
    {
        std::cout << "Triangle vertex shader successfully loaded" << std::endl;
    }

    // build the pipeline layout that controls the inputs and outputs of the shader
    // i'm not using descriptor sets or other systems yet, so no need to use anything other than empty defaults
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = VulkanInitialisers::PipelineLayoutCreateInfo();

    VK_CHECK(vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_trianglePipelineLayout));

    // build the stage creation info for both vertex and fragment stages.
    // this lets the pipeline know the shader modules per stage
    PipelineBuilder pipelineBuilder;

    // add vertex shader stage
    pipelineBuilder.ShaderStages.push_back(
            VulkanInitialisers::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, triangleVertexShader)
    );

    // add fragment shader stage
    pipelineBuilder.ShaderStages.push_back(
            VulkanInitialisers::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragmentShader)
    );

    // vertex input controls how to read vertices from vertex buffers, not using it yet
    pipelineBuilder.VertexInputInfo = VulkanInitialisers::VertexInputStateCreateInfo();

    // input assembly is the configuration for drawing triangle lists, strips or individual points
    pipelineBuilder.InputAssembly = VulkanInitialisers::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // build viewport and scissor from the swapchain extents
    pipelineBuilder.Viewport.x = 0.0f;
    pipelineBuilder.Viewport.y = 0.0f;
    pipelineBuilder.Viewport.width = (float) _windowExtent.width;
    pipelineBuilder.Viewport.height = (float) _windowExtent.height;
    pipelineBuilder.Viewport.minDepth = 0.0f;
    pipelineBuilder.Viewport.maxDepth = 1.0f;

    pipelineBuilder.Scissor.offset = {0, 0};
    pipelineBuilder.Scissor.extent = _windowExtent;

    // configure the rasteriser to draw full triangles
    pipelineBuilder.Rasteriser = VulkanInitialisers::RasterisationStateCreateInfo(VK_POLYGON_MODE_LINE);

    // default multisampling (1 sample per pixel)
    pipelineBuilder.Multisampling = VulkanInitialisers::MultisamplingStateCreateInfo();

    // single blend attachment with no blending and writing to RGBA
    pipelineBuilder.ColourBlendAttachment = VulkanInitialisers::ColorBlendAttachmentState();

    // triangle layout
    pipelineBuilder.PipelineLayout = _trianglePipelineLayout;

    // woot, lets build this fucker
    _trianglePipeline = pipelineBuilder.BuildPipeline(_device, _renderPass);

}

bool VulkanEngine::LoadShaderModule(const char *filePath, VkShaderModule *outShaderModule)
{
    // open the shader file with the cursor at the end (ios::ate) and in binary mode (ios::binary)
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        return false;
    }

    // determine the file size by looking at the location of the cursor. Because the cursor is at the end of the file,
    // we get the size directly in bytes
    size_t fileSize = (size_t) file.tellg();

    // spirv expects the bugger to be in uint32, so we need to make sure to reserve an int vector big enough for the
    // entire shader file
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    // reset cursor to beginning of file
    file.seekg(0);

    // load the entire file into the buffer
    file.read((char *) buffer.data(), fileSize);

    // now that the file is loaded into the buffer, we can close it like the tidy kiwi I am ;)
    file.close();

    // create a new shader module, using the above buffer
    VkShaderModuleCreateInfo createInfo = {}; // initialise struct with 0's
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;

    // codeSize has to be in bytes
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();

    // confirm creation goes well
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        return false;
    }

    *outShaderModule = shaderModule;
    return true;
}