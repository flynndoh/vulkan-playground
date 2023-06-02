#include <iostream>
#include "vk_engine.h"

int main()
{
    std::cout << "Welcome to my Vulkan Playground!" << std::endl;

    VulkanEngine engine;
    engine.Init();
    engine.Run();
    engine.Cleanup();

    return 0;
}
