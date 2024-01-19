#pragma once

#include "VulkanTypes.h"

#include <SDL_video.h>
#include <vector>

namespace vulkan_engine
{

class VulkanEngine
{
  public:
    // initializes everything in the engine
    void init();

    // shuts down the engine
    void cleanup() const;

    // draw loop
    void draw();

    // run main loop
    void run();

  private:
    void init_vulcan();

    void init_swapchain();

    void init_commands();

    void init_default_render_pass();

    VkAttachmentDescription create_colour_attachment();

    static VkSubpassDescription create_subpass();

    void init_framebuffers();

    void init_sync_structures();

    void init_pipelines();

    bool load_shader_module(const char *filePath, VkShaderModule *outShaderModule);

  private:
    VkInstance _instance;                      // Vulkan library handle
    VkDebugUtilsMessengerEXT _debug_messenger; // Vulkan debug output handle
    VkPhysicalDevice _chosen_gpu;              // GPU chosen as the default hardware device
    VkDevice _device;                          // Logical Vulkan device for commands
    VkSurfaceKHR _surface;                     // Vulkan window service

    VkSwapchainKHR _swapchain;
    VkFormat _swapchain_image_format; // image format expected by the windowing system
    std::vector<VkImage> _swapchain_images;
    std::vector<VkImageView> _swapchain_image_views;

    VkQueue _graphics_queue;         // queue that all render jobs will be submitted to
    uint32_t _graphics_queue_family; // the above queue's family type

    VkCommandPool _command_pool;
    VkCommandBuffer _main_command_buffer; // the buffer that we will record into

    VkRenderPass _render_pass;
    std::vector<VkFramebuffer> _framebuffers;

    VkSemaphore _present_semaphore, _render_semaphore;
    VkFence _render_fence;

    VkPipelineLayout _triangle_pipeline_layout;
    VkPipeline _rainbow_triangle_pipeline;
    VkPipeline _red_triangle_pipeline;

    VkExtent2D _window_extent{640, 320};
    SDL_Window *_window{nullptr};
    bool _is_initialized{false};
    int _frame_number{0};
    int _selected_shader{0};
};
} // namespace vulkan_engine