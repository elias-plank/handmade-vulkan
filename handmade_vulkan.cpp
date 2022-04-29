#include "handmade_vulkan.h"

#ifdef _DEBUG
static const bool EnableValidationLayers = true;
#else
static const bool EnableValidationLayers = false;
#endif

namespace handmade {
	
	static const char* ValidationLayers[] = { "VK_LAYER_KHRONOS_validation" };
	static const char* DeviceExtensions[] = { "VK_KHR_swapchain" };
	static const u32 FramesInFlight = 2;
	
	static bool VulkanCheckValidationLayerSupport() {
		
		// Check for validation-layer support
		u32 layerCount{};
		VkLayerProperties* availableLayers{};
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		availableLayers = (VkLayerProperties*)malloc(layerCount * sizeof(VkLayerProperties));
		
		if (availableLayers) {
			
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);
			
			bool found = false;
			for (u32 i = 0; i < layerCount; i++) {
				
				VkLayerProperties* layer = (availableLayers + i);
				
				if (strcmp(*ValidationLayers, layer->layerName) == 0) {
					
					found = true;
					break;
				}
			}
			
			free(availableLayers);
			return found;
		}
		
		return false;
	}
	
	static bool VulkanCreateInstance(VulkanState* state) {
		
		// If validation-layers are enabled (Debug-Mode), we need to check if our machine supports them
		if (EnableValidationLayers && !VulkanCheckValidationLayerSupport()) {
			
			return false;
		}
		
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Handmade Vulkan";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Vulkan Engine";
		appInfo.apiVersion = VK_API_VERSION_1_1;
		
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledLayerCount = 0;
		
		if (EnableValidationLayers) {
			
			const char* debugExtensions[] = { "VK_EXT_debug_utils", "VK_KHR_surface", "VK_KHR_win32_surface" };
			createInfo.enabledExtensionCount = ARRAY_SIZE(debugExtensions);
			createInfo.ppEnabledExtensionNames = debugExtensions;
			createInfo.enabledLayerCount = ARRAY_SIZE(ValidationLayers);
			createInfo.ppEnabledLayerNames = ValidationLayers;
		}
		else {
			
			const char* extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
			createInfo.enabledExtensionCount = ARRAY_SIZE(extensions);
			createInfo.ppEnabledExtensionNames = extensions;
			createInfo.enabledLayerCount = 0;
		}
		
		{
			// Print the extensions
			u32 extensionCount{};
			VkExtensionProperties* extensions{};
			
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
			extensions = (VkExtensionProperties*)malloc(extensionCount * sizeof(VkExtensionProperties));
			
			if (extensions) {
				
				vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions);
				
				fprintf(stdout, "[Vulkan] - Available Extensions:\n");
				for (u32 i = 0; i < extensionCount; i++) {
					
					VkExtensionProperties* property = (extensions + i);
					fprintf(stdout, "\t%s\n", property->extensionName);
				}
				free(extensions);
			}
		}
		
		return vkCreateInstance(&createInfo, nullptr, &state->Instance) == VK_SUCCESS;
	}
	
	static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
															  VkDebugUtilsMessageSeverityFlagBitsEXT severity,
															  VkDebugUtilsMessageTypeFlagsEXT type,
															  const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
															  void* userData) {
		
		if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			
			fprintf(stderr, "[Vulkan] [Validation] - %s}\n", callbackData->pMessage);
		}
		
		return VK_FALSE;
	}
	
	static VkResult VulkanCreateDebugUtilsMessengerEXT(
													   VkInstance instance, 
													   const VkDebugUtilsMessengerCreateInfoEXT* createInfo, 
													   const VkAllocationCallbacks* allocator, 
													   VkDebugUtilsMessengerEXT* messenger) {
		
		PFN_vkCreateDebugUtilsMessengerEXT function{};
		function = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		
		if (function != nullptr) {
			
			return function(instance, createInfo, allocator, messenger);
		}
		else {
			
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}
	
	static void VulkanDestroyDebugUtilsMessengerEXT(
													VkInstance instance, 
													VkDebugUtilsMessengerEXT messenger, 
													const VkAllocationCallbacks* allocator) {
		
		PFN_vkDestroyDebugUtilsMessengerEXT function{};
		function = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		
		if (function != nullptr) {
			
			function(instance, messenger, allocator);
		}
	}
	
	static bool VulkanCreateDebugMessenger(VulkanState* state) {
		
		if (!EnableValidationLayers) {
			
			return true;
		}
		
		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = 
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = 
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = VulkanDebugCallback;
		createInfo.pUserData = nullptr;
		
		if (VulkanCreateDebugUtilsMessengerEXT(state->Instance, &createInfo, nullptr, &state->DebugMessenger) != VK_SUCCESS) {
			
			return false;
		}
		
		return true;
	}
	
	static bool VulkanCreateSurface(VulkanState* state) {
		
		VkWin32SurfaceCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = glfwGetWin32Window(state->Window->NativeHandle);
		createInfo.hinstance = GetModuleHandleA(nullptr);
		
		return vkCreateWin32SurfaceKHR(state->Instance, &createInfo, nullptr, &state->Surface) == VK_SUCCESS;
	}
	
	static VulkanQueueFamilyIndices VulkanFindQueueFamilies(VulkanState* state, VkPhysicalDevice* device) {
		
		VulkanQueueFamilyIndices indices{};
		
		u32 queueFamilyCount{};
		VkQueueFamilyProperties* queueFamilies{};
		vkGetPhysicalDeviceQueueFamilyProperties(*device, &queueFamilyCount, nullptr);
		queueFamilies = (VkQueueFamilyProperties*)malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
		
		if (queueFamilies) {
			
			vkGetPhysicalDeviceQueueFamilyProperties(*device, &queueFamilyCount, queueFamilies);
			
			for (u32 i = 0; i < queueFamilyCount; i++) {
				
				VkQueueFamilyProperties* queueFamily = (queueFamilies + i);
				if (queueFamily->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					
					indices.GraphicsFamily = i;
					indices.GraphicsComplete = true;
				}
				
				VkBool32 presentSupport{};
				vkGetPhysicalDeviceSurfaceSupportKHR(*device, i, state->Surface, &presentSupport);
				
				if (presentSupport) {
					
					indices.PresentFamily = i;
					indices.PresentComplete = true;
				}
				
				if (indices.GraphicsComplete && indices.PresentComplete) {
					
					break;
				}
			}
			free(queueFamilies);
		}
		
		return indices;
	}
	
	static bool VulkanCheckDeviceExtensionSupport(VkPhysicalDevice* device) {
		
		u32 extensionCount{};
		VkExtensionProperties* extensions{};
		vkEnumerateDeviceExtensionProperties(*device, nullptr, &extensionCount, nullptr);
		
		extensions = (VkExtensionProperties*)malloc(extensionCount * sizeof(VkExtensionProperties));
		
		if (extensions) {
			
			vkEnumerateDeviceExtensionProperties(*device, nullptr, &extensionCount, extensions);
			
			bool requiredExtensionFound{};
			for (u32 i = 0; i < extensionCount; i++) {
				
				VkExtensionProperties* extension = (extensions + i);
				if (strcmp(*DeviceExtensions, extension->extensionName) == 0) {
					
					requiredExtensionFound = true;
					break;
				}
			}
			
			free(extensions);
			return requiredExtensionFound;
		}
		else {
			
			return false;
		}
	}
	
	static VulkanSwapChainSupportDetails VulkanQuerySwapChainSupport(VulkanState* state, VkPhysicalDevice* device) {
		
		VulkanSwapChainSupportDetails details{};
		
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*device, state->Surface, &details.Capabilities);
		
		u32 formatCount{};
		vkGetPhysicalDeviceSurfaceFormatsKHR(*device, state->Surface, &formatCount, nullptr);
		if (formatCount != 0) {
			
			details.Formats = (VkSurfaceFormatKHR*)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
			details.FormatCount = formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(*device, state->Surface, &formatCount, details.Formats);
		}
		
		u32 presentModeCount{};
		vkGetPhysicalDeviceSurfacePresentModesKHR(*device, state->Surface, &presentModeCount, nullptr);
		if (presentModeCount != 0) {
			
			details.PresentModes = (VkPresentModeKHR*)malloc(presentModeCount * sizeof(VkPresentModeKHR));
			details.PresentModeCount = presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(*device, state->Surface, &presentModeCount, details.PresentModes);
		}
		
		return details;
	}
	
	static bool VulkanIsPhysicalDeviceCapable(VulkanState* state, VkPhysicalDevice* device) {
		
		VulkanQueueFamilyIndices indices = VulkanFindQueueFamilies(state, device);
		
		bool extensions = VulkanCheckDeviceExtensionSupport(device);
		bool swapChainAdequate{};
		if (extensions) {
			
			VulkanSwapChainSupportDetails swapChainSupport = VulkanQuerySwapChainSupport(state, device);
			swapChainAdequate = swapChainSupport.FormatCount > 0 && swapChainSupport.PresentModeCount > 0;
			
			free(swapChainSupport.Formats);
			free(swapChainSupport.PresentModes);
		}
		
		return indices.GraphicsComplete && indices.PresentComplete && extensions && swapChainAdequate;
	}
	
	static bool VulkanPickPhysicalDevice(VulkanState* state) {
		
		u32 deviceCount{};
		VkPhysicalDevice* devices{};
		vkEnumeratePhysicalDevices(state->Instance, &deviceCount, nullptr);
		
		if (deviceCount == 0) {
			
			return false;
		}
		
		devices = (VkPhysicalDevice*)malloc(deviceCount * sizeof(VkPhysicalDevice));
		
		if (devices) {
			
			vkEnumeratePhysicalDevices(state->Instance, &deviceCount, devices);
			
			for (u32 i = 0; i < deviceCount; i++) {
				
				VkPhysicalDevice* device = (devices + i);
				if (VulkanIsPhysicalDeviceCapable(state, device)) {
					
					state->PhysicalDevice = *device;
					break;
				}
			}
			
			free(devices);
			return state->PhysicalDevice != VK_NULL_HANDLE;
		}
		else {
			
			return false;
		}
	}
	
	static bool VulkanCreateLogicalDevice(VulkanState* state) {
		
		VulkanQueueFamilyIndices indices = VulkanFindQueueFamilies(state, &state->PhysicalDevice);
		
		u32 createInfoCount = indices.GraphicsFamily != indices.PresentFamily ? 2 : 1;
		VkDeviceQueueCreateInfo* createInfos{};
		createInfos = (VkDeviceQueueCreateInfo*)malloc(createInfoCount * sizeof(VkDeviceQueueCreateInfo));
		
		if (createInfos) {
			
			f32 queuePriority = 1.0f;
			VkDeviceQueueCreateInfo queueCreateInfoGraphics{};
			queueCreateInfoGraphics.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfoGraphics.queueFamilyIndex = indices.GraphicsFamily;
			queueCreateInfoGraphics.queueCount = 1;
			queueCreateInfoGraphics.pQueuePriorities = &queuePriority;
			memcpy(&createInfos[0], &queueCreateInfoGraphics, sizeof(queueCreateInfoGraphics));
			
			if (createInfoCount == 2) {
				
				VkDeviceQueueCreateInfo queueCreateInfoPresent{};
				queueCreateInfoGraphics.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfoGraphics.queueFamilyIndex = indices.PresentFamily;
				queueCreateInfoGraphics.queueCount = 1;
				queueCreateInfoGraphics.pQueuePriorities = &queuePriority;
				memcpy(&createInfos[1], &queueCreateInfoPresent, sizeof(queueCreateInfoPresent));
			}
			
			VkPhysicalDeviceFeatures deviceFeatures{};
			VkDeviceCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo.queueCreateInfoCount = createInfoCount;
			createInfo.pQueueCreateInfos = createInfos;
			createInfo.pEnabledFeatures = &deviceFeatures;
			createInfo.enabledExtensionCount = ARRAY_SIZE(DeviceExtensions);
			createInfo.ppEnabledExtensionNames = DeviceExtensions;
			
			if (EnableValidationLayers) {
				
				createInfo.enabledLayerCount = ARRAY_SIZE(ValidationLayers);
				createInfo.ppEnabledLayerNames = ValidationLayers;
			}
			
			VkResult result = vkCreateDevice(state->PhysicalDevice, &createInfo, nullptr, &state->Device);
			free(createInfos);
			
			if (result != VK_SUCCESS) {
				
				return false;
			}
			
			vkGetDeviceQueue(state->Device, indices.GraphicsFamily, 0, &state->GraphicsQueue);
			vkGetDeviceQueue(state->Device, indices.PresentFamily, 0, &state->PresentQueue);
			
			return true;
		}
		else {
			
			return false;
		}
	}
	
	static VkSurfaceFormatKHR VulkanChooseSwapSurfaceFormat(VkSurfaceFormatKHR* formats, u32 count) {
		
		for (u32 i = 0; i < count; i++) {
			
			VkSurfaceFormatKHR* format = (formats + i);
			if (format->format == VK_FORMAT_B8G8R8A8_SRGB && format->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				
				return *format;
			}
		}
		
		return *formats;
	}
	
	static VkPresentModeKHR VulkanChooseSwapPresentMode(VkPresentModeKHR* presentModes, u32 count) {
		
		for (u32 i = 0; i < count; i++) {
			
			// Check if 'tripple-buffering' is available
			VkPresentModeKHR* presentMode = (presentModes + i);
			if (*presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				
				return *presentMode;
			}
		}
		
		return VK_PRESENT_MODE_FIFO_KHR;
	}
	
	static VkExtent2D VulkanChooseSwapExtent(VkSurfaceCapabilitiesKHR* capabilities, Window* window) {
		
		if (capabilities->currentExtent.width != UINT32_MAX) {
			
			return capabilities->currentExtent;
		}
		else {
			
			i32 width{};
			i32 height{};
			glfwGetFramebufferSize(window->NativeHandle, &width, &height);
			
			VkExtent2D extent{};
			extent.width = (u32)width;
			extent.height = (u32)height;
			
			f32 minWidth = (f32)capabilities->minImageExtent.width;
			f32 minHeight = (f32)capabilities->minImageExtent.height;
			f32 maxWidth = (f32)capabilities->maxImageExtent.width;
			f32 maxHeight = (f32)capabilities->maxImageExtent.height;
			
			extent.width = (u32)Clamp(extent.width, minWidth, maxWidth);
			extent.height = (u32)Clamp(extent.height, minHeight, maxHeight);
			
			return extent;
		}
	}
	
	static bool VulkanCreateSwapChain(VulkanState* state) {
		
		VulkanSwapChainSupportDetails swapChainSupport = VulkanQuerySwapChainSupport(state, &state->PhysicalDevice);
		
		VkSurfaceFormatKHR surfaceFormat = VulkanChooseSwapSurfaceFormat(swapChainSupport.Formats, swapChainSupport.FormatCount);
		VkPresentModeKHR presentMode = VulkanChooseSwapPresentMode(swapChainSupport.PresentModes, swapChainSupport.PresentModeCount);
		VkExtent2D extent = VulkanChooseSwapExtent(&swapChainSupport.Capabilities, state->Window);
		free(swapChainSupport.Formats);
		free(swapChainSupport.PresentModes);
		
		u32 imageCount = swapChainSupport.Capabilities.minImageCount + 1;
		if (swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount) {
			
			imageCount = swapChainSupport.Capabilities.maxImageCount;
		}
		
		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = state->Surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		
		VulkanQueueFamilyIndices indices = VulkanFindQueueFamilies(state, &state->PhysicalDevice);
		u32 queueFamilyIndices[2] = { indices.GraphicsFamily, indices.PresentFamily };
		
		if (indices.GraphicsFamily != indices.PresentFamily) {
			
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}
		
		createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;
		
		VkResult result = vkCreateSwapchainKHR(state->Device, &createInfo, nullptr, &state->SwapChain.SwapChain);
		if (result != VK_SUCCESS) {
			
			return false;
		}
		
		vkGetSwapchainImagesKHR(state->Device, state->SwapChain.SwapChain, &imageCount, nullptr);
		state->SwapChain.Images = (VkImage*)malloc(imageCount * sizeof(VkImage));
		
		if (state->SwapChain.Images) {
			
			vkGetSwapchainImagesKHR(state->Device, state->SwapChain.SwapChain, &imageCount, state->SwapChain.Images);
			state->SwapChain.ImageFormat = surfaceFormat.format;
			state->SwapChain.Extent = extent;
			state->SwapChain.ImageCount = imageCount;
			
			return true;
		}
		else {
			
			return false;
		}
	}
	
	static bool VulkanCreateImageViews(VulkanState* state) {
		
		state->SwapChain.ImageViews = (VkImageView*)malloc(state->SwapChain.ImageCount * sizeof(VkImageView));
		state->SwapChain.ImageViewCount = state->SwapChain.ImageCount;
		
		bool complete = true;
		for (u32 i = 0; i < state->SwapChain.ImageCount; i++) {
			
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = *(state->SwapChain.Images + i);
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = state->SwapChain.ImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;
			
			VkImageView* imageView = (state->SwapChain.ImageViews + i);
			if (vkCreateImageView(state->Device, &createInfo, nullptr, imageView) != VK_SUCCESS) {
				
				complete = false;
			}
		}
		
		return complete;
	}
	
	static VkShaderModule VulkanCreateShaderModule(VulkanState* state, VulkanShaderCode* code) {
		
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code->Size;
		createInfo.pCode = (const u32*)code->Data;
		
		VkShaderModule shaderModule{};
		if (vkCreateShaderModule(state->Device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			
			fprintf(stderr, "Couldn't create shader!");
		}
		
		return shaderModule;
	}
	
	static bool VulkanCreateRenderPass(VulkanState* state) {
		
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = state->SwapChain.ImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		
		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		
		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;
		
		return vkCreateRenderPass(state->Device, &renderPassInfo, nullptr, &state->Pipeline.RenderPass) == VK_SUCCESS;
	}
	
	static bool VulkanCreateGraphicsPipeline(VulkanState* state) {
		
		VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
		vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderStageInfo.module = state->Shader.VertexShader;
		vertexShaderStageInfo.pName = "main";
		
		VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
		fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderStageInfo.module = state->Shader.FragmentShader;
		fragmentShaderStageInfo.pName = "main";
		
		VkPipelineShaderStageCreateInfo shaderStages[2] = { vertexShaderStageInfo, fragmentShaderStageInfo };
		
		VertexDescription vertexDescription = VertexGetDescription();
		
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = ARRAY_SIZE(vertexDescription.Attributes);
		vertexInputInfo.pVertexBindingDescriptions = &vertexDescription.Binding;
		vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.Attributes;
		
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;
		
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (f32)state->SwapChain.Extent.width;
		viewport.height = (f32)state->SwapChain.Extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = state->SwapChain.Extent;
		
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;
		
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;
		
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;
		
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = 
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional
		
		VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };
		
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = ARRAY_SIZE(dynamicStates);
		dynamicState.pDynamicStates = dynamicStates;
		
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;
		
		VkResult pipelineLayoutResult = vkCreatePipelineLayout(state->Device, &pipelineLayoutInfo, nullptr, &state->Pipeline.PipeLineLayout);
		
		if (pipelineLayoutResult == VK_SUCCESS) {
			
			VkGraphicsPipelineCreateInfo pipelineInfo{};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.stageCount = 2;
			pipelineInfo.pStages = shaderStages;
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizer;
			pipelineInfo.pMultisampleState = &multisampling;
			pipelineInfo.pDepthStencilState = nullptr;
			pipelineInfo.pColorBlendState = &colorBlending;
			pipelineInfo.pDynamicState = nullptr;
			pipelineInfo.layout = state->Pipeline.PipeLineLayout;
			pipelineInfo.renderPass = state->Pipeline.RenderPass;
			pipelineInfo.subpass = 0;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
			pipelineInfo.basePipelineIndex = -1;
			
			VkResult pipelineResult = vkCreateGraphicsPipelines(state->Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &state->Pipeline.GraphicsPipeline);
			
			return pipelineResult == VK_SUCCESS;
		}
		
		return false;
	}
	
	static bool VulkanCreateFramebuffers(VulkanState* state) {
		
		state->SwapChain.Framebuffers = (VkFramebuffer*)malloc(state->SwapChain.ImageViewCount * sizeof(VkFramebuffer));
		state->SwapChain.FramebufferCount = state->SwapChain.ImageViewCount;
		
		if (state->SwapChain.Framebuffers) {
			
			bool complete = true;
			for (u32 i = 0; i < state->SwapChain.ImageViewCount; i++) {
				
				VkImageView* imageView = (state->SwapChain.ImageViews + i);
				VkFramebufferCreateInfo framebufferInfo{};
				framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebufferInfo.renderPass = state->Pipeline.RenderPass;
				framebufferInfo.attachmentCount = 1;
				framebufferInfo.pAttachments = imageView;
				framebufferInfo.width = state->SwapChain.Extent.width;
				framebufferInfo.height = state->SwapChain.Extent.height;
				framebufferInfo.layers = 1;
				
				if (vkCreateFramebuffer(state->Device, &framebufferInfo, nullptr, &state->SwapChain.Framebuffers[i]) != VK_SUCCESS) {
					
					complete = false;
				}
			}
			
			return complete;
		}
		else {
			
			return false;
		}
	}// NOTE(Elias): 
	
	static bool VulkanCreateCommandPool(VulkanState* state) {
		
		VulkanQueueFamilyIndices queueFamilyIndices = VulkanFindQueueFamilies(state, &state->PhysicalDevice);
		
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily;
		
		return vkCreateCommandPool(state->Device, &poolInfo, nullptr, &state->CommandPool) == VK_SUCCESS;
	}
	
	static u32 VulkanFindMemoryType(VulkanState* state, u32 typeFilter, VkMemoryPropertyFlags properties) {
		
		VkPhysicalDeviceMemoryProperties memProperties{};
		vkGetPhysicalDeviceMemoryProperties(state->PhysicalDevice, &memProperties);
		
		for (u32 i = 0; i < memProperties.memoryTypeCount; i++) {
			
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				
				return i + 1;
			}
		}
		
		return 0;
	}
	
	static bool VulkanCreateBuffer(VulkanState* state, VulkanBuffer* buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
		
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		
		if (vkCreateBuffer(state->Device, &bufferInfo, nullptr, &buffer->Buffer) != VK_SUCCESS) {
			
			return false;
		}
		
		VkMemoryRequirements memRequirements{};
		vkGetBufferMemoryRequirements(state->Device, buffer->Buffer, &memRequirements);
		
		u32 memoryTypeIndex = VulkanFindMemoryType(state, memRequirements.memoryTypeBits, properties);
		
		if (memoryTypeIndex == 0) {
			
			return false;
		}
		
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = memoryTypeIndex - 1;
		
		if (vkAllocateMemory(state->Device, &allocInfo, nullptr, &buffer->BufferMemory) != VK_SUCCESS) {
			
			return false;
		}
		
		vkBindBufferMemory(state->Device, buffer->Buffer, buffer->BufferMemory, 0);
		
		return true;
	}
	
	static bool VulkanCopyBuffer(VulkanState* state, VkBuffer source, VkBuffer destination, VkDeviceSize size) {
		
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = state->CommandPool;
		allocInfo.commandBufferCount = 1;
		
		VkCommandBuffer commandBuffer{};
		vkAllocateCommandBuffers(state->Device, &allocInfo, &commandBuffer);
		
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		
		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		
		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, source, destination, 1, &copyRegion);
		
		vkEndCommandBuffer(commandBuffer);
		
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		
		vkQueueSubmit(state->GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(state->GraphicsQueue);
		
		vkFreeCommandBuffers(state->Device, state->CommandPool, 1, &commandBuffer);
		
		return true;
	}
	
	static bool VulkanCreateCommandBuffers(VulkanState* state) {
		
		state->CommandBuffers = (VkCommandBuffer*)malloc(FramesInFlight * sizeof(VkCommandBuffer));
		state->CommandBufferCount = FramesInFlight;
		
		if (state->CommandBuffers) {
			
			VkCommandBufferAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = state->CommandPool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = state->CommandBufferCount;
			
			return vkAllocateCommandBuffers(state->Device, &allocInfo, state->CommandBuffers) == VK_SUCCESS;
		}
		else {
			
			return false;
		}
	}
	
	static bool VulkanRecordCommandBuffer(VulkanState* state, VkCommandBuffer commandBuffer, u32 imageIndex, VkBuffer vertexBuffer, VkBuffer indexBuffer, u32 indexCount) {
		
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		
		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			
			return false;
		}
		
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = state->Pipeline.RenderPass;
		renderPassInfo.framebuffer = *(state->SwapChain.Framebuffers + imageIndex);
		renderPassInfo.renderArea.offset = { 0 , 0 };
		renderPassInfo.renderArea.extent = state->SwapChain.Extent;
		
		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
		
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->Pipeline.GraphicsPipeline);
			
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
		}
		vkCmdEndRenderPass(commandBuffer);
		
		return vkEndCommandBuffer(commandBuffer) == VK_SUCCESS;
	}
	
	static bool VulkanCreateSyncObjects(VulkanState* state) {
		
		state->ImageAvailableSemaphores = (VkSemaphore*)malloc(FramesInFlight * sizeof(VkSemaphore));
		state->ImageAvailableSemaphoreCount = FramesInFlight;
		
		state->RenderFinishedSemaphores = (VkSemaphore*)malloc(FramesInFlight * sizeof(VkSemaphore));
		state->RenderFinishedSemaphoreCount = FramesInFlight;
		
		state->InFlightFences = (VkFence*)malloc(FramesInFlight * sizeof(VkFence));
		state->InFlightFenceCount = FramesInFlight;
		
		if (state->ImageAvailableSemaphores && state->RenderFinishedSemaphores && state->InFlightFences) {
			
			VkSemaphoreCreateInfo semaphoreInfo{};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			
			VkFenceCreateInfo fenceInfo{};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			
			bool complete = true;
			for (u32 i = 0; i < FramesInFlight; i++) {
				
				VkSemaphore* imageAvailableSemaphore = (state->ImageAvailableSemaphores + i);
				VkSemaphore* renderFinishedSemaphore = (state->RenderFinishedSemaphores + i);
				VkFence* inFlightFence = (state->InFlightFences + i);
				
				VkResult imageAvailable = vkCreateSemaphore(state->Device, &semaphoreInfo, nullptr, imageAvailableSemaphore);
				VkResult renderFinished = vkCreateSemaphore(state->Device, &semaphoreInfo, nullptr, renderFinishedSemaphore);
				VkResult inFlight = vkCreateFence(state->Device, &fenceInfo, nullptr, inFlightFence);
				
				if (imageAvailable != VK_SUCCESS || renderFinished != VK_SUCCESS || inFlight != VK_SUCCESS) {
					
					complete = false;
					break;
				}
			}
			
			return complete;
		}
		else {
			
			return false;
		}
	}
	
	static void VulkanFramebufferResizeCallback(GLFWwindow* window, i32 width, i32 height) {
		
		VulkanState* state = (VulkanState*)glfwGetWindowUserPointer(window);
		state->SwapChain.FramebufferResized;
	}
	
	static bool VulkanCreateFramebufferResizeCallback(VulkanState* state) {
		
		glfwSetWindowUserPointer(state->Window->NativeHandle, state);
		glfwSetFramebufferSizeCallback(state->Window->NativeHandle, VulkanFramebufferResizeCallback);
	}
	
	static void VulkanCleanupSwapChain(VulkanState* state) {
		
		vkDeviceWaitIdle(state->Device);
		
		// Framebuffers
		for (u32 i = 0; i < state->SwapChain.FramebufferCount; i++) {
			
			VkFramebuffer* framebuffer = (state->SwapChain.Framebuffers + i);
			vkDestroyFramebuffer(state->Device, *framebuffer, nullptr);
		}
		free(state->SwapChain.Framebuffers);
		
		// Pipeline
		vkDestroyPipeline(state->Device, state->Pipeline.GraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(state->Device, state->Pipeline.PipeLineLayout, nullptr);
		vkDestroyRenderPass(state->Device, state->Pipeline.RenderPass, nullptr);
		
		// Image Views
		for (u32 i = 0; i < state->SwapChain.ImageViewCount; i++) {
			
			VkImageView* imageView = (state->SwapChain.ImageViews + i);
			vkDestroyImageView(state->Device, *imageView, nullptr);
		}
		free(state->SwapChain.ImageViews);
		
		// SwapChain
		vkDestroySwapchainKHR(state->Device, state->SwapChain.SwapChain, nullptr);
		free(state->SwapChain.Images);
	}
	
	static bool VulkanRecreateSwapChain(VulkanState* state) {
		
		// Wait while the window size is zero
		i32 width{};
		i32 height{};
		glfwGetFramebufferSize(state->Window->NativeHandle, &width, &height);
		while (width == 0 || height == 0) {
			
			glfwGetFramebufferSize(state->Window->NativeHandle, &width, &height);
			glfwWaitEvents();
		}
		
		VulkanCleanupSwapChain(state);
		
		u32 result = 1;
		
		result &= (u32)VulkanCreateSwapChain(state);
		result &= (u32)VulkanCreateImageViews(state);
		result &= (u32)VulkanCreateRenderPass(state);
		result &= (u32)VulkanCreateGraphicsPipeline(state);
		result &= (u32)VulkanCreateFramebuffers(state);
		
		return result;
	}
	
	bool VulkanStateInit(VulkanState* state, Window* window) {
		
		u32 result = 1;
		state->Window = window;
		
		result &= (u32)VulkanCreateInstance(state);
		result &= (u32)VulkanCreateDebugMessenger(state);
		result &= (u32)VulkanCreateSurface(state);
		result &= (u32)VulkanPickPhysicalDevice(state);
		result &= (u32)VulkanCreateLogicalDevice(state);
		result &= (u32)VulkanCreateSwapChain(state);
		result &= (u32)VulkanCreateImageViews(state);
		result &= (u32)VulkanCreateRenderPass(state);
		
		// At this point we want to load the default Shader
		VulkanShader defaultShader{};
		result &= (u32)VulkanCreateShader(
										  state, 
										  &defaultShader,
										  "assets/handmade_triangle_vert.spv",
										  "assets/handmade_triangle_frag.spv");
		
		state->Shader = defaultShader;
		state->DefaultShader = defaultShader;
		
		result &= (u32)VulkanCreateGraphicsPipeline(state);
		result &= (u32)VulkanCreateFramebuffers(state);
		result &= (u32)VulkanCreateCommandPool(state);
		result &= (u32)VulkanCreateCommandBuffers(state);
		result &= (u32)VulkanCreateSyncObjects(state);
		
		return result;
	}
	
	bool VulkanStateDestroy(VulkanState* state) {
		
		// Swap Chain
		VulkanCleanupSwapChain(state);
		
		// Destroy the default shader
		VulkanDestroyShader(state, &state->DefaultShader);
		
		// Sync Objects
		for (u32 i = 0; i < FramesInFlight; i++) {
			
			VkSemaphore* imageAvailableSemaphore = (state->ImageAvailableSemaphores + i);
			VkSemaphore* renderFinishedSemaphore = (state->RenderFinishedSemaphores + i);
			VkFence* inFlightFence = (state->InFlightFences + i);
			
			vkDestroySemaphore(state->Device, *renderFinishedSemaphore, nullptr);
			vkDestroySemaphore(state->Device, *imageAvailableSemaphore, nullptr);
			vkDestroyFence(state->Device, *inFlightFence, nullptr);
		}
		free(state->ImageAvailableSemaphores);
		free(state->RenderFinishedSemaphores);
		
		vkDestroyCommandPool(state->Device, state->CommandPool, nullptr);
		free(state->CommandBuffers);
		
		// Device
		vkDestroyDevice(state->Device, nullptr);
		
		// Debug Messenger
		if (EnableValidationLayers) {
			
			VulkanDestroyDebugUtilsMessengerEXT(state->Instance, state->DebugMessenger, nullptr);
		}
		
		// Instance and Surface
		vkDestroySurfaceKHR(state->Instance, state->Surface, nullptr);
		vkDestroyInstance(state->Instance, nullptr);
		
		return true;
	}
	
	bool VulkanCreateVertexBuffer(VulkanState* state, VulkanBuffer* vertexBuffer, Vertex* vertices, u32 count) {
		
		VkDeviceSize bufferSize = count * sizeof(Vertex);
		VulkanBuffer stagingBuffer{};
		
		// Create the staging buffer
		if (!VulkanCreateBuffer(state, &stagingBuffer, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
			
			return false;
		}
		
		void* data;
		vkMapMemory(state->Device, stagingBuffer.BufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices, (size_t)bufferSize);
		vkUnmapMemory(state->Device, stagingBuffer.BufferMemory);
		
		// Create the vertex buffer
		if (!VulkanCreateBuffer(state, vertexBuffer, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
			
			return false;
		}
		
		if (!VulkanCopyBuffer(state, stagingBuffer.Buffer, vertexBuffer->Buffer, bufferSize)) {
			
			return false;
		}
		
		vkDestroyBuffer(state->Device, stagingBuffer.Buffer, nullptr);
		vkFreeMemory(state->Device, stagingBuffer.BufferMemory, nullptr);
		
		return true;
	}
	
	void VulkanDestroyVertexBuffer(VulkanState* state, VulkanBuffer* vertexBuffer) {
		
		vkDeviceWaitIdle(state->Device);
		vkDestroyBuffer(state->Device, vertexBuffer->Buffer, nullptr);
		vkFreeMemory(state->Device, vertexBuffer->BufferMemory, nullptr);
	}
	
	bool VulkanVertexBufferSetData(VulkanState* state, VulkanBuffer* vertexBuffer, Vertex* vertices, u32 count) {
		
		VkDeviceSize bufferSize = count * sizeof(Vertex);
		VulkanBuffer stagingBuffer{};
		
		// Create the staging buffer
		if (!VulkanCreateBuffer(state, &stagingBuffer, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
			
			return false;
		}
		
		void* data;
		vkMapMemory(state->Device, stagingBuffer.BufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices, (size_t)bufferSize);
		vkUnmapMemory(state->Device, stagingBuffer.BufferMemory);
		
		if (!VulkanCopyBuffer(state, stagingBuffer.Buffer, vertexBuffer->Buffer, bufferSize)) {
			
			return false;
		}
		
		vkDestroyBuffer(state->Device, stagingBuffer.Buffer, nullptr);
		vkFreeMemory(state->Device, stagingBuffer.BufferMemory, nullptr);
		
		return true;
	}
	
	bool VulkanCreateIndexBuffer(VulkanState* state, VulkanBuffer* indexBuffer, u32* indices, u32 count) {
		
		VkDeviceSize bufferSize = count * sizeof(u32);
		VulkanBuffer stagingBuffer{};
		
		if (!VulkanCreateBuffer(state, &stagingBuffer, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
			
			return false;
		}
		
		void* data;
		vkMapMemory(state->Device, stagingBuffer.BufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices, (size_t)bufferSize);
		vkUnmapMemory(state->Device, stagingBuffer.BufferMemory);
		
		if (!VulkanCreateBuffer(state, indexBuffer, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
			
			return false;
		}
		
		if (!VulkanCopyBuffer(state, stagingBuffer.Buffer, indexBuffer->Buffer, bufferSize)) {
			
			return false;
		}
		
		vkDestroyBuffer(state->Device, stagingBuffer.Buffer, nullptr);
		vkFreeMemory(state->Device, stagingBuffer.BufferMemory, nullptr);
		
		return true;
	}
	
	void VulkanDestroyIndexBuffer(VulkanState* state, VulkanBuffer* indexBuffer) {
		
		vkDeviceWaitIdle(state->Device);
		vkDestroyBuffer(state->Device, indexBuffer->Buffer, nullptr);
		vkFreeMemory(state->Device, indexBuffer->BufferMemory, nullptr);
	}
	
	bool VulkanIndexBufferSetData(VulkanState* state, VulkanBuffer* indexBuffer, u32* indices, u32 count) {
		
		VkDeviceSize bufferSize = count * sizeof(u32);
		VulkanBuffer stagingBuffer{};
		
		if (!VulkanCreateBuffer(state, &stagingBuffer, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
			
			return false;
		}
		
		void* data;
		vkMapMemory(state->Device, stagingBuffer.BufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices, (size_t)bufferSize);
		vkUnmapMemory(state->Device, stagingBuffer.BufferMemory);
		
		if (!VulkanCopyBuffer(state, stagingBuffer.Buffer, indexBuffer->Buffer, bufferSize)) {
			
			return false;
		}
		
		vkDestroyBuffer(state->Device, stagingBuffer.Buffer, nullptr);
		vkFreeMemory(state->Device, stagingBuffer.BufferMemory, nullptr);
		
		return true;
	}
	
	bool VulkanDrawIndexed(VulkanState* state, VulkanBuffer* vertexBuffer, VulkanBuffer* indexBuffer, u32 indexCount) {
		
		VkSemaphore* imageAvailableSemaphore = (state->ImageAvailableSemaphores + state->CurrentFrame);
		VkSemaphore* renderFinishedSemaphore = (state->RenderFinishedSemaphores + state->CurrentFrame);
		VkFence* inFlightFence = (state->InFlightFences + state->CurrentFrame);
		VkCommandBuffer* commandBuffer = (state->CommandBuffers + state->CurrentFrame);
		
		vkWaitForFences(state->Device, 1, inFlightFence, VK_TRUE, UINT64_MAX);
		
		u32 imageIndex{};
		VkResult result = vkAcquireNextImageKHR(state->Device, state->SwapChain.SwapChain, UINT64_MAX, *imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			
			state->SwapChain.FramebufferResized = false;
			VulkanRecreateSwapChain(state);
			return true;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			
			return false;
		}
		
		vkResetFences(state->Device, 1, inFlightFence);
		
		vkResetCommandBuffer(*commandBuffer, 0);
		VulkanRecordCommandBuffer(state, *commandBuffer, imageIndex, vertexBuffer->Buffer, indexBuffer->Buffer, indexCount);
		
		VkPipelineStageFlags waitStages[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = imageAvailableSemaphore;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = commandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = renderFinishedSemaphore;
		
		if (vkQueueSubmit(state->GraphicsQueue, 1, &submitInfo, *inFlightFence) != VK_SUCCESS) {
			
			return false;
		}
		
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = renderFinishedSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &state->SwapChain.SwapChain;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;
		
		vkQueuePresentKHR(state->PresentQueue, &presentInfo);
		
		state->CurrentFrame = (state->CurrentFrame + 1) & FramesInFlight;
		
		return true;
	}
	
	bool VulkanCreateShader(VulkanState* state, VulkanShader* shader, const char* vertexPath, const char* fragmentPath) {
		
		VulkanShaderCode vertexCode = VulkanLoadShaderCode(vertexPath);
		VulkanShaderCode fragmentCode = VulkanLoadShaderCode(fragmentPath);
		
		shader->VertexShader = VulkanCreateShaderModule(state, &vertexCode);
		shader->FragmentShader = VulkanCreateShaderModule(state, &fragmentCode);
		
		free(vertexCode.Data);
		free(fragmentCode.Data);
		
		return true;
	}
	
	bool VulkanUseShader(VulkanState* state, VulkanShader* shader) {
		
		state->Shader = *shader;
		return VulkanRecreateSwapChain(state);
	}
	
	void VulkanDestroyShader(VulkanState* state, VulkanShader* shader) {
		
		vkDestroyShaderModule(state->Device, shader->VertexShader, nullptr);
		vkDestroyShaderModule(state->Device, shader->FragmentShader, nullptr);
	}
	
	VulkanShaderCode VulkanLoadShaderCode(const char* path) {
		
		VulkanShaderCode shaderCode{};
		
		FILE* file = fopen(path, "rb");
		if (file) {
			
			u32 size{};
			fseek(file, 0, SEEK_END);
			size = (u32)ftell(file);
			fseek(file, 0, SEEK_SET);
			
			shaderCode.Data = (u8*)malloc(size * sizeof(u8));
			
			if (shaderCode.Data) {
				
				shaderCode.Size = size;
				fread(shaderCode.Data, sizeof(u8), size, file);
			}
			
			fclose(file);
		}
		
		return shaderCode;
	}
}
