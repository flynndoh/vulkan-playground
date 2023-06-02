// cpp-vulkan.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>
#include <vector>

class VulkanEngine {
public:

    VkInstance _instance; // Vulkan library handle
    VkDebugUtilsMessengerEXT _debugMessenger; // Vulkan debug output handle
    VkPhysicalDevice _chosenGPU; // GPU chosen as the default hardware device
    VkDevice _device; // Logical Vulkan device for commands
    VkSurfaceKHR _surface; // Vulkan window service

    VkSwapchainKHR _swapchain;
    VkFormat _swapchainImageFormat; // image format expected by the windowing system
    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;

    VkExtent2D _windowExtent{1700, 900};

    struct SDL_Window *_window{nullptr};

    bool _isInitialized{false};
    int _frameNumber{0};

    //initializes everything in the engine
    void Init();

    //shuts down the engine
    void Cleanup() const;

    //draw loop
    void Draw();

    //run main loop
    void Run();

private:

    void InitVulcan();

    void InitSwapchain();
};
