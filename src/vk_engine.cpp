#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <VkBootstrap.h>

#include "vk_engine.h"

// We want to immediately abort when there is an error. In normal engines this would give an error message to the user,
// or perform a dump of state.
#define VK_CHECK(x)                                                         \
    using namespace std                                                     \
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
            "Vulkan Engine",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            _windowExtent.width,
            _windowExtent.height,
            window_flags
    );

    // load the core Vulkan structures
    InitVulcan();

    // create the swap chain (frame buffering)
    InitSwapchain();

    // everything went fine
    _isInitialized = true;
}

void VulkanEngine::Cleanup() const
{
    // NOTE: We must destroy objects in the reverse order in which they were created
    if (_isInitialized)
    {
        vkDestroySwapchainKHR(_device, _swapchain, nullptr);

        // destroy swapchain resources
        for (auto _swapchainImageView: _swapchainImageViews)
        {
            vkDestroyImageView(_device, _swapchainImageView, nullptr);
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
    // nothing yet
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
