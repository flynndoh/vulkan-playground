#include <iostream>
#include "VulkanEngine.h"

int main()
{
    std::cout << "Starting engine" << std::endl;
    VulkanEngine engine;
    engine.Init();

    std::cout << "Running engine" << std::endl;
    engine.Run();

    std::cout << "Cleaning up engine" << std::endl;
    engine.Cleanup();

    return 0;
}
