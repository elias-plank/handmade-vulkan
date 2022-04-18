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

	struct VulkanPipeline {

		VkRenderPass RenderPass;
		VkPipelineLayout PipeLineLayout;
		VkPipeline GraphicsPipeline;
	};

	struct VulkanState {

		VkInstance Instance;
		VkDebugUtilsMessengerEXT DebugMessenger;
		
		VkPhysicalDevice PhysicalDevice;
		VkDevice Device;
		VkSurfaceKHR Surface;
		
		VkQueue GraphicsQueue;
		VkQueue PresentQueue;

		VkSwapchainKHR SwapChain;
		VkImage* SwapChainImages;
		u32 SwapChainImageCount;
		VkFormat SwapChainImageFormat;
		VkExtent2D SwapChainExtent;
		VkImageView* SwapChainImageViews;
		u32 SwapChainImageViewCount;

		VulkanPipeline Pipeline;

		VkFramebuffer* SwapChainFramebuffers;
		u32 SwapChainFramebufferCount;

		VkCommandPool CommandPool;
		VkCommandBuffer CommandBuffer;

		VkSemaphore ImageAvailableSemaphore;
		VkSemaphore RenderFinishedSemaphore;
		VkFence InFlightFence;
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

	bool VulkanStateInit(VulkanState* state, Window* window);
	bool VulkanStateDestroy(VulkanState* state);
	bool VulkanStateDrawFrame(VulkanState* state);

	VulkanShaderCode VulkanLoadShaderCode(const char* path);
}

#endif //HANDMADE_VULKAN_H
