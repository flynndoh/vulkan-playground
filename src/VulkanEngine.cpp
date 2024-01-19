#include "VulkanEngine.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <VkBootstrap.h>

#include <cmath>
#include <fstream>
#include <iostream>

#include "PipelineBuilder.h"
#include "VulkanInitialisers.h"

namespace vulkan_engine
{
// We want to immediately abort when there is an error. In normal engines this
// would give an error message to the user, or perform a dump of state.
#define VK_CHECK(x)                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        VkResult err = x;                                                                                              \
        if (err)                                                                                                       \
        {                                                                                                              \
            std::cout << "Detected Vulkan Error: " << err << std::endl;                                                \
            abort();                                                                                                   \
        }                                                                                                              \
    } while (0)

void VulkanEngine::init()
{
    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

    auto window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    _window = SDL_CreateWindow("rendering window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                               _window_extent.width, _window_extent.height, window_flags);

    // load the core Vulkan structures
    init_vulcan();

    // create the swap chain (image buffers to be used when frame buffering)
    init_swapchain();

    // create the queue to facilitate sending commands to the GPU
    init_commands();

    init_default_render_pass();

    init_framebuffers();

    // create fences to sync comms from the GPU to the CPU. Create semaphores to
    // sync comms between GPU and GPU
    init_sync_structures();

    init_pipelines();

    // everything went fine
    _is_initialized = true;
}

void VulkanEngine::cleanup() const
{
    // NOTE: We must destroy objects in the reverse order in which they were
    // created
    if (_is_initialized)
    {
        // make sure the GPU has stopped doing its things
        vkDeviceWaitIdle(_device);

        // destroy command pool
        vkDestroyCommandPool(_device, _command_pool, nullptr);

        // destroy sync objects
        vkDestroyFence(_device, _render_fence, nullptr);
        vkDestroySemaphore(_device, _render_semaphore, nullptr);
        vkDestroySemaphore(_device, _present_semaphore, nullptr);

        // destroy swapchain
        vkDestroySwapchainKHR(_device, _swapchain, nullptr);

        // destroy the main render pass
        vkDestroyRenderPass(_device, _render_pass, nullptr);

        // destroy swapchain resources
        for (int i = 0; i < _swapchain_image_views.size(); ++i)
        {
            vkDestroyFramebuffer(_device, _framebuffers[i], nullptr);

            vkDestroyImageView(_device, _swapchain_image_views[i], nullptr);
        }

        vkDestroyDevice(_device, nullptr);
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
        vkDestroyInstance(_instance, nullptr);

        SDL_DestroyWindow(_window);
    }
}

void VulkanEngine::draw()
{
    // wait until the GPU has finished rendering the previous frame, with a
    // timeout of 1 second
    VK_CHECK(vkWaitForFences(_device, 1, &_render_fence, true, 1000000000 /*ns*/));
    VK_CHECK(vkResetFences(_device, 1, &_render_fence));

    // request an image from the swapchain, with a timeout of 1 second
    uint32_t swapchain_image_index;
    VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000 /*ns*/, _present_semaphore, nullptr,
                                   &swapchain_image_index));

    // at this point, we are sure that the commands have finished executing, and
    // we can safely reset the command buffer before we begin recording to it
    // again
    VK_CHECK(vkResetCommandBuffer(_main_command_buffer, 0));

    VkCommandBuffer command_buffer = _main_command_buffer;

    // begin the command buffer recording. We will use this command buffer exactly
    // one time, so we want to let Vulkan know that
    VkCommandBufferBeginInfo command_buffer_begin_info = {}; // initialise structure to 0's
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = nullptr;
    command_buffer_begin_info.pInheritanceInfo = nullptr;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));

    // make a clear colour from frame number. This will flash with a 120*pi frame
    // period
    VkClearValue clear_value;
    float flash = abs(std::sin(_frame_number / 120.f));
    clear_value.color = {{0.0f, 0.0f, flash, 1.0f}};

    // start with the main render pass
    // we will use the clear colour defined above, and the framebuffer of the
    // index the swapchain gave us
    VkRenderPassBeginInfo render_pass_begin_info = {}; // initialise struct with 0's
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = _render_pass;
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.renderArea.extent = _window_extent;
    render_pass_begin_info.framebuffer = _framebuffers[swapchain_image_index]; // this is where we render into

    // connect up clear values
    render_pass_begin_info.clearValueCount = 1;
    render_pass_begin_info.pClearValues = &clear_value;

    // begin this render pass
    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    // render commands go here
    if (_selected_shader == 0)
    {
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _rainbow_triangle_pipeline);
    }
    else
    {
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _red_triangle_pipeline);
    }

    vkCmdDraw(command_buffer, 3, 1, 0, 0);

    // finalise this render pass
    vkCmdEndRenderPass(command_buffer);

    // finalise command buffer (we can no longer add commands, but it can now be
    // executed by the GPU)
    VK_CHECK(vkEndCommandBuffer(command_buffer));

    // prepare submision to the queue
    // we want to wait on the _present_semaphore, as that semaphore is signaled
    // when the swapchain is ready we will signal the _render_semaphore to signal
    // that rendering has finished
    VkSubmitInfo submit = {}; // initialise struct to 0's
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.pNext = nullptr;

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit.pWaitDstStageMask = &wait_stage;

    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &_present_semaphore;

    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &_render_semaphore;

    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command_buffer;

    // submit the command bugger to the queue and execute it
    // _render_fence will now block until the GPU finishes executing the graphics
    // commands
    VK_CHECK(vkQueueSubmit(_graphics_queue, 1, &submit, _render_fence));

    // this will put the image we just rendered into the visible window
    // we want to wait on the _render_semaphore for that as it's necessary that all
    // drawing commands have finished before the image is displayed to the user
    VkPresentInfoKHR present_info = {}; // initialise struct to 0's
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;

    present_info.swapchainCount = 1;
    present_info.pSwapchains = &_swapchain;

    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &_render_semaphore;

    present_info.pImageIndices = &swapchain_image_index;

    // present to user's screen
    VK_CHECK(vkQueuePresentKHR(_graphics_queue, &present_info));

    // increment the number of frames drawn
    _frame_number++;
}

void VulkanEngine::run()
{
    SDL_Event e;
    bool b_quit = false;

    // main loop
    while (!b_quit)
    {
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0)
        {
            // close the window when user alt-f4s or clicks the X button
            if (e.type == SDL_QUIT)
            {
                b_quit = true;
            }
            else if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_SPACE)
                {
                    _selected_shader += 1;
                    if (_selected_shader > 1)
                    {
                        _selected_shader = 0;
                    }
                }
            }
        }

        draw();
    }
}

void VulkanEngine::init_vulcan()
{
    vkb::InstanceBuilder builder;

    auto instance = builder.set_app_name("Vulkan Playground")
                        .request_validation_layers(true) // TODO: In prod, we would remove
                                                         // this to improve performance
                        .require_api_version(1, 1, 0)
                        .use_default_debug_messenger() // Use this to catch validation errors
                        .build();

    vkb::Instance vkb_instance = instance.value();

    // persist the instance for reuse
    _instance = vkb_instance.instance;

    // also persist debug messenger
    _debug_messenger = vkb_instance.debug_messenger;

    // create a surface using the window we opened with SDL
    SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

    // select a physical GPU to use, using the vk bootstrap library to choose for
    // us
    vkb::PhysicalDeviceSelector selector{vkb_instance};
    vkb::PhysicalDevice physical_device = selector.set_minimum_version(1, 1).set_surface(_surface).select().value();

    // create the logical Vulkan device using the selected physical GPU
    vkb::DeviceBuilder device_builder{physical_device};
    vkb::Device vkb_device = device_builder.build().value();

    // persist for later usage
    _device = vkb_device.device;
    _chosen_gpu = physical_device.physical_device;

    // use bootstrapper to get a graphics queue
    _graphics_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    _graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
}

void VulkanEngine::init_swapchain()
{
    vkb::SwapchainBuilder swapchain_builder{_chosen_gpu, _device, _surface};

    vkb::Swapchain vkb_swapchain = swapchain_builder.use_default_format_selection()
                                       .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // TODO: vsync present mode
                                       .set_desired_extent(_window_extent.width,
                                                           _window_extent.height) // TODO: Need to rebuild the swapchain
                                                                                  // whenever the window is resized
                                       .build()
                                       .value();

    // persist swapchain and related image stuff
    _swapchain = vkb_swapchain.swapchain;
    _swapchain_images = vkb_swapchain.get_images().value();
    _swapchain_image_views = vkb_swapchain.get_image_views().value();
    _swapchain_image_format = vkb_swapchain.image_format;
}

void VulkanEngine::init_commands()
{
    // create command pool for commands submitted to the graphics queue
    // we also want the pool to allow for resetting of individual command buffers
    VkCommandPoolCreateInfo command_pool_info = vulkan_engine::initialisers::command_pool_create_info(
        _graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_CHECK(vkCreateCommandPool(_device, &command_pool_info, nullptr, &_command_pool));

    // allocate the default command buffer that we will use for rendering
    VkCommandBufferAllocateInfo command_alloc_info =
        vulkan_engine::initialisers::command_buffer_allocate_info(_command_pool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    VK_CHECK(vkAllocateCommandBuffers(_device, &command_alloc_info, &_main_command_buffer));
}

void VulkanEngine::init_default_render_pass()
{
    VkAttachmentDescription colour_attachment = create_colour_attachment();
    VkSubpassDescription single_subpass = create_subpass();

    VkRenderPassCreateInfo render_pass_info = {}; // initialise struct to 0's
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

    // connect the colour attachment to the render pass info
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &colour_attachment;

    // connect the single subpass to the render pass info
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &single_subpass;

    VK_CHECK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_render_pass));
}

VkAttachmentDescription VulkanEngine::create_colour_attachment()
{
    // the render pass will use this colour attachment
    VkAttachmentDescription colour_attachment = {};

    // same attachment will have the format needed by the swapchain
    colour_attachment.format = _swapchain_image_format;

    // 1 sample, can do higher samples for MSAA
    colour_attachment.samples = VK_SAMPLE_COUNT_1_BIT;

    // clear when this attachment is loaded
    colour_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

    // we keep the attachment stored when the render pass ends
    colour_attachment.storeOp - VK_ATTACHMENT_STORE_OP_STORE;

    // dont care about stencil
    colour_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colour_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // we don't know, nor do we care, about the starting layout of the attachment
    colour_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // once the render pass ends, the image needs to be in a layout that is ready
    // for display
    colour_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    return colour_attachment;
}

VkSubpassDescription VulkanEngine::create_subpass()
{
    VkAttachmentReference attachment_reference = {}; // initialise the struct with 0's

    // attachment number will index into the pAttachments array in the parent
    // render pass itself
    attachment_reference.attachment = 0;
    attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // only want to create 1 subpass, which is the minimum number you cna do
    VkSubpassDescription subpass = {}; // initialise the struct with 0's
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachment_reference;

    return subpass;
}

void VulkanEngine::init_framebuffers()
{
    // create the framebuffers for the swapchain images. This will connect the
    // render pass to the images for rendering
    VkFramebufferCreateInfo frame_buffer_info = {}; // initialise struct with 0's
    frame_buffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frame_buffer_info.pNext = nullptr;

    frame_buffer_info.renderPass = _render_pass;
    frame_buffer_info.attachmentCount = 1;
    frame_buffer_info.width = _window_extent.width;
    frame_buffer_info.height = _window_extent.height;
    frame_buffer_info.layers = 1;

    // grab the number of images we have in the swapchain
    const uint32_t swapchain_image_count = _swapchain_images.size();
    _framebuffers = std::vector<VkFramebuffer>(swapchain_image_count);

    // create a framebuffer for each of the swapchain image views
    for (int i = 0; i < swapchain_image_count; ++i)
    {
        frame_buffer_info.pAttachments = &_swapchain_image_views[i];
        VK_CHECK(vkCreateFramebuffer(_device, &frame_buffer_info, nullptr, &_framebuffers[i]));
    }
}

void VulkanEngine::init_sync_structures()
{
    // create sync structures
    VkFenceCreateInfo fence_info = {}; // initialise structure with 0's
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.pNext = nullptr;

    // create the GPU --> CPU fence with the "CREATE_SIGNALED" flag, so that we
    // can wait on it before using it on a GPU command (for the first frame)
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CHECK(vkCreateFence(_device, &fence_info, nullptr, &_render_fence));

    // for the semaphores, we don't need much setup
    VkSemaphoreCreateInfo semaphore_info = {}; // initialise structure with 0's
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_info.pNext = nullptr;
    semaphore_info.flags = 0;

    // create presentation semaphore
    VK_CHECK(vkCreateSemaphore(_device, &semaphore_info, nullptr, &_present_semaphore));

    // create rendering semaphore
    VK_CHECK(vkCreateSemaphore(_device, &semaphore_info, nullptr, &_render_semaphore));
}

void VulkanEngine::init_pipelines()
{
    VkShaderModule red_triangle_fragment_shader; // TODO: this is currently leaked
    if (!load_shader_module("../shaders/triangle.frag.spv", &red_triangle_fragment_shader))
    {
        std::cout << "Error when building the red triangle fragment shader module" << std::endl;
    }
    else
    {
        std::cout << "Red triangle fragment shader successfully loaded" << std::endl;
    }

    VkShaderModule red_triangle_vertex_shader; // TODO: this is currently leaked
    if (!load_shader_module("../shaders/triangle.vert.spv", &red_triangle_vertex_shader))
    {
        std::cout << "Error when building the red triangle vertex shader module" << std::endl;
    }
    else
    {
        std::cout << "Red triangle vertex shader successfully loaded" << std::endl;
    }

    VkShaderModule rainbow_triangle_fragment_shader; // TODO: this is currently leaked
    if (!load_shader_module("../shaders/rainbowTriangle.frag.spv", &rainbow_triangle_fragment_shader))
    {
        std::cout << "Error when building the rainbow triangle fragment shader module" << std::endl;
    }
    else
    {
        std::cout << "Rainbow triangle fragment shader successfully loaded" << std::endl;
    }

    VkShaderModule rainbow_triangle_vertex_shader; // TODO: this is currently leaked
    if (!load_shader_module("../shaders/rainbowTriangle.vert.spv", &rainbow_triangle_vertex_shader))
    {
        std::cout << "Error when building the rainbow triangle vertex shader module" << std::endl;
    }
    else
    {
        std::cout << "Rainbow triangle vertex shader successfully loaded" << std::endl;
    }

    // build the pipeline layout that controls the inputs and outputs of the shader i'm not using descriptor sets or
    // other systems yet, so no need to use anything other than empty defaults
    VkPipelineLayoutCreateInfo pipeline_layout_info = vulkan_engine::initialisers::pipeline_layout_create_info();

    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_triangle_pipeline_layout));

    // build the stage creation info for both vertex and fragment stages.
    // this lets the pipeline know the shader modules per stage
    PipelineBuilder pipeline_builder;

    // add vertex shader stage
    pipeline_builder.shader_stages.push_back(vulkan_engine::initialisers::pipeline_shader_stage_create_info(
        VK_SHADER_STAGE_VERTEX_BIT, rainbow_triangle_vertex_shader));

    // add fragment shader stage
    pipeline_builder.shader_stages.push_back(vulkan_engine::initialisers::pipeline_shader_stage_create_info(
        VK_SHADER_STAGE_FRAGMENT_BIT, rainbow_triangle_fragment_shader));

    // vertex input controls how to read vertices from vertex buffers, not using it yet
    pipeline_builder.vertex_input_info = vulkan_engine::initialisers::vertex_input_state_create_info();

    // input assembly is the configuration for drawing triangle lists, strips or
    // individual points
    pipeline_builder.input_assembly =
        vulkan_engine::initialisers::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // build viewport and scissor from the swapchain extents
    pipeline_builder.viewport.x = 0.0f;
    pipeline_builder.viewport.y = 0.0f;
    pipeline_builder.viewport.width = (float)_window_extent.width;
    pipeline_builder.viewport.height = (float)_window_extent.height;
    pipeline_builder.viewport.minDepth = 0.0f;
    pipeline_builder.viewport.maxDepth = 1.0f;

    pipeline_builder.scissor.offset = {0, 0};
    pipeline_builder.scissor.extent = _window_extent;

    // configure the rasteriser to draw full triangles
    pipeline_builder.rasteriser = vulkan_engine::initialisers::rasterisation_state_create_info(VK_POLYGON_MODE_FILL);

    // default multisampling (1 sample per pixel)
    pipeline_builder.multisampling = vulkan_engine::initialisers::multisampling_state_create_info();

    // single blend attachment with no blending and writing to RGBA
    pipeline_builder.colour_blend_attachment = vulkan_engine::initialisers::color_blend_attachment_state();

    // triangle layout
    pipeline_builder.pipeline_layout = _triangle_pipeline_layout;

    // woot, lets build the rainbow triangle pipeline
    _rainbow_triangle_pipeline = pipeline_builder.build_pipeline(_device, _render_pass);

    // now we want to build another pipeline for the static red triangle
    // first we need to clear the existing shader stages from the other triangle
    // pipeline
    pipeline_builder.shader_stages.clear();

    // add the shaders for the static red triangle
    pipeline_builder.shader_stages.push_back(vulkan_engine::initialisers::pipeline_shader_stage_create_info(
        VK_SHADER_STAGE_VERTEX_BIT, red_triangle_vertex_shader));

    // add fragment shader stage
    pipeline_builder.shader_stages.push_back(vulkan_engine::initialisers::pipeline_shader_stage_create_info(
        VK_SHADER_STAGE_FRAGMENT_BIT, red_triangle_fragment_shader));

    // build the static red triangle pipeline
    _red_triangle_pipeline = pipeline_builder.build_pipeline(_device, _render_pass);
}

bool VulkanEngine::load_shader_module(const char *filePath, VkShaderModule *outShaderModule)
{
    // open the shader file with the cursor at the end (ios::ate) and in binary
    // mode (ios::binary)
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        return false;
    }

    // determine the file size by looking at the location of the cursor. Because
    // the cursor is at the end of the file, we get the size directly in bytes
    auto file_size = (size_t)file.tellg();

    // spirv expects the bugger to be in uint32, so we need to make sure to
    // reserve an int vector big enough for the entire shader file
    std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));

    // reset cursor to beginning of file
    file.seekg(0);

    // load the entire file into the buffer
    file.read((char *)buffer.data(), file_size);

    // now that the file is loaded into the buffer, we can close it like the tidy
    // kiwi I am ;)
    file.close();

    // create a new shader module, using the above buffer
    VkShaderModuleCreateInfo create_info = {}; // initialise struct with 0's
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.pNext = nullptr;

    // codeSize has to be in bytes
    create_info.codeSize = buffer.size() * sizeof(uint32_t);
    create_info.pCode = buffer.data();

    // confirm creation goes well
    VkShaderModule shader_module;
    if (vkCreateShaderModule(_device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
    {
        return false;
    }

    *outShaderModule = shader_module;
    return true;
};
} // namespace vulkan_engine