#include "handmade_vulkan.h"
#include "handmade_window.h"

namespace handmade {
	
	int Main(int argc, char** argv) {
		
		Window window{};
		if (WindowCreate(&window, "Handmade Vulkan", 800, 600)) {
			
			// Initialize the vulkan state
			VulkanState vulkanState{};
			if (VulkanStateInit(&vulkanState, &window)) {
				
				Vertex vertices[4] = {
					
					{{-1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},
					{{ 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
					{{ 1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}},
					{{-1.0f,  1.0f}, {1.0f, 1.0f, 1.0f}}
				};
				
				u32 indices[6] = {
					
					0, 1, 2, 2, 3, 0
				};
				
				u32 indexCount = ARRAY_SIZE(indices);
				
				VulkanBuffer vertexBuffer{};
				VulkanCreateVertexBuffer(&vulkanState, &vertexBuffer, vertices, ARRAY_SIZE(vertices));
				
				VulkanBuffer indexBuffer{};
				VulkanCreateIndexBuffer(&vulkanState, &indexBuffer, indices, ARRAY_SIZE(indices));
				
				VulkanShader redShader{};
				VulkanCreateShader(&vulkanState, &redShader, "assets/handmade_red_vert.spv", "assets/handmade_red_frag.spv");
				VulkanUseShader(&vulkanState, &redShader);
				
				f64 frameTime = glfwGetTime();
				u64 inc = 0;
				
				while (WindowIsRunning(&window)) {
					
					f64 time = glfwGetTime();
					f64 dt = time - frameTime;
					frameTime = time;
					
					if (inc++ % 1000 == 0) {
						
						printf("fps: %lf\r", 1.0 / dt);
					}
					
					VulkanDrawIndexed(&vulkanState,&vertexBuffer, &indexBuffer, indexCount);
					WindowUpdate(&window);
				}
				
				VulkanDestroyVertexBuffer(&vulkanState, &vertexBuffer);
				VulkanDestroyIndexBuffer(&vulkanState, &indexBuffer);
				VulkanDestroyShader(&vulkanState, &redShader);
			}
			
			VulkanStateDestroy(&vulkanState);
		}
		
		WindowDestroy(&window);
		
		return 0;
	}
}

int main(int argc, char** argv) {
	
	return handmade::Main(argc, argv);
}
