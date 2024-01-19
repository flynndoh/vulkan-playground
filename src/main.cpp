#include <iostream>

#include "VulkanEngine.h"

int main()
{
    std::cout << "Starting engine" << std::endl;
    vulkan_engine::VulkanEngine engine;
    engine.init();

    std::cout << "Running engine" << std::endl;
    engine.run();

    std::cout << "Cleaning up engine" << std::endl;
    engine.cleanup();

    return 0;
}
