#include "handmade_vulkan.h"
#include "handmade_window.h"

int main(int argc, char** argv) {

	// Window
	handmade::Window window{};
	if (handmade::WindowCreate(&window, "Handmade Vulkan", 800, 600)) {

		// Vulkan
		handmade::VulkanState vulkanState{};
		if (handmade::VulkanStateInit(&vulkanState, &window)) {

			while (window.Running) {

				handmade::WindowUpdate(&window);
			}
		}

		handmade::VulkanStateDestroy(&vulkanState);
	}
	
	handmade::WindowDestroy(&window);

	return 0;
}
