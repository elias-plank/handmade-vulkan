#include "handmade_vulkan.h"
#include "handmade_window.h"

namespace handmade {
    
	int Main(int argc, char** argv) {
        
		Window window{};
		if (WindowCreate(&window, "Handmade Vulkan", 800, 600)) {
            
			// Vulkan
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
                
				VulkanBuffer vertexBuffer{};
				VulkanCreateVertexBuffer(&vulkanState, &vertexBuffer, vertices, ARRAY_SIZE(vertices));
                
				VulkanBuffer indexBuffer{};
				VulkanCreateIndexBuffer(&vulkanState, &indexBuffer, indices, ARRAY_SIZE(indices));
                
                f64 frameTime = glfwGetTime();
                u64 inc = 0;
                
				while (WindowIsRunning(&window)) {
                    
                    f64 time = glfwGetTime();
                    f64 dt = time - frameTime;
                    frameTime = time;
                    
                    if (inc % 1000 == 0) {
                        
                        printf("fps: %lf\r", 1.0 / dt);
                    }
                    
                    inc++;
                    
                    if (GetAsyncKeyState(VK_F4)) {
                        
                        Vertex adjustedVertices[4] = {
                            
                            {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                            {{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
                            {{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
                            {{-0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}}
                        };
                        
                        VulkanVertexBufferSetData(&vulkanState, &vertexBuffer, adjustedVertices, 4);
                    }
                    
					VulkanDrawIndexed(&vulkanState, &window, &vertexBuffer, &indexBuffer, ARRAY_SIZE(indices));
					WindowUpdate(&window);
				}
                
				VulkanDestroyVertexBuffer(&vulkanState, &vertexBuffer);
				VulkanDestroyIndexBuffer(&vulkanState, &indexBuffer);
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
