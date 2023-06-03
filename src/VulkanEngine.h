// cpp-vulkan.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <VulkanTypes.h>
#include <vector>

class VulkanEngine {

public:
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

    void InitCommands();

    void InitDefaultRenderPass();

    VkAttachmentDescription CreateColourAttachment();

    static VkSubpassDescription CreateSubpass();

    void InitFramebuffers();

    void InitSyncStructures();

private:
    VkInstance _instance; // Vulkan library handle
    VkDebugUtilsMessengerEXT _debugMessenger; // Vulkan debug output handle
    VkPhysicalDevice _chosenGPU; // GPU chosen as the default hardware device
    VkDevice _device; // Logical Vulkan device for commands
    VkSurfaceKHR _surface; // Vulkan window service

    VkSwapchainKHR _swapchain;
    VkFormat _swapchainImageFormat; // image format expected by the windowing system
    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;

    VkQueue _graphicsQueue; // queue that all render jobs will be submitted to
    uint32_t _graphicsQueueFamily; // the above queue's family type

    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer; // the buffer that we will record into

    VkRenderPass _renderPass;
    std::vector<VkFramebuffer> _framebuffers;

    VkSemaphore _presentSemaphore, _renderSemaphore;
    VkFence _renderFence;

    VkExtent2D _windowExtent{640, 380};

    struct SDL_Window *_window{nullptr};
    bool _isInitialized{false};
    int _frameNumber{0};

};
