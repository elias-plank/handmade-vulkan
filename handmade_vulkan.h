/* date = April 12th 2022 11:38 pm */

#ifndef HANDMADE_VULKAN_H
#define HANDMADE_VULKAN_H

#include "handmade_types.h"
#include "handmade_math.h"
#include "handmade_window.h"

#pragma warning(disable : 26812)
#include <vulkan/vulkan.h>
#include <cstdlib>

namespace handmade {
	
	struct VulkanShaderCode {
		
		u8* Data;
		u32 Size;
	};
	
	struct VulkanShader {
		
		VkShaderModule VertexShader;
		VkShaderModule FragmentShader;
	};
	
	struct VulkanSwapChain {
		
		VkSwapchainKHR SwapChain;
		VkImage* Images;
		u32 ImageCount;
		VkFormat ImageFormat;
		VkExtent2D Extent;
		VkImageView* ImageViews;
		u32 ImageViewCount;
		
		VkFramebuffer* Framebuffers;
		u32 FramebufferCount;
		bool FramebufferResized;
	};
	
	struct VulkanPipeline {
		
		VkRenderPass RenderPass;
		VkPipelineLayout PipeLineLayout;
		VkPipeline GraphicsPipeline;
	};
	
	struct VulkanBuffer {
		
		VkBuffer Buffer;
		VkDeviceMemory BufferMemory;
	};
	
	struct VulkanState {
		
		VkInstance Instance;
		VkDebugUtilsMessengerEXT DebugMessenger;
		
		VkPhysicalDevice PhysicalDevice;
		VkDevice Device;
		VkSurfaceKHR Surface;
		
		VkQueue GraphicsQueue;
		VkQueue PresentQueue;
		
		VulkanSwapChain SwapChain;
		VulkanPipeline Pipeline;
		
		VkCommandPool CommandPool;
		VkCommandBuffer* CommandBuffers;
		u32 CommandBufferCount;
		
		VkSemaphore* ImageAvailableSemaphores;
		u32 ImageAvailableSemaphoreCount;
		VkSemaphore* RenderFinishedSemaphores;
		u32 RenderFinishedSemaphoreCount;
		VkFence* InFlightFences;
		u32 InFlightFenceCount;
		u32 CurrentFrame;
		
		VulkanShader Shader;
		VulkanShader DefaultShader;
		
		Window* Window;
	};
	
	struct VulkanQueueFamilyIndices {
		
		u32 GraphicsFamily;
		u32 PresentFamily;
		bool GraphicsComplete;
		bool PresentComplete;
	};
	
	struct VulkanSwapChainSupportDetails {
		
		VkSurfaceCapabilitiesKHR Capabilities;
		VkSurfaceFormatKHR* Formats;
		VkPresentModeKHR* PresentModes;
		u32 FormatCount;
		u32 PresentModeCount;
	};
	
	// State Functions
	bool VulkanStateInit(VulkanState* state, Window* window);
	bool VulkanStateDestroy(VulkanState* state);
	
	// Drawing
	bool VulkanDrawIndexed(VulkanState* state, VulkanBuffer* vertexBuffer, VulkanBuffer* indexBuffer, u32 indexCount);
	
	bool VulkanCreateVertexBuffer(VulkanState* state, VulkanBuffer* vertexBuffer, Vertex* vertices, u32 count);
	void VulkanDestroyVertexBuffer(VulkanState* state, VulkanBuffer* vertexBuffer);
	bool VulkanVertexBufferSetData(VulkanState* state, VulkanBuffer* vertexBuffer, Vertex* vertices, u32 count);
	
	bool VulkanCreateIndexBuffer(VulkanState* state, VulkanBuffer* indexBuffer, u32* indices, u32 count);
	void VulkanDestroyIndexBuffer(VulkanState* state, VulkanBuffer* indexBuffer);
	bool VulkanIndexBufferSetData(VulkanState* state, VulkanBuffer* indexBuffer, u32* indices, u32 count);
	
	bool VulkanCreateShader(VulkanState* state, VulkanShader* shader, const char* vertexPath, const char* fragmentPath);
	bool VulkanUseShader(VulkanState* state, VulkanShader* shader);
	void VulkanDestroyShader(VulkanState* state, VulkanShader* shader);
	
	VulkanShaderCode VulkanLoadShaderCode(const char* path);
}

#endif //HANDMADE_VULKAN_H
