#include "handmade_vulkan.h"

#ifdef _DEBUG
static const bool EnableValidationLayers = true;
#else
static const bool EnableValidationLayers = false;
#endif

static const char* ValidationLayers[] = { "VK_LAYER_KHRONOS_validation" };
static const char* DeviceExtensions[] = { "VK_KHR_swapchain" };

namespace handmade {

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
		appInfo.apiVersion = VK_API_VERSION_1_3;

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

				std::fprintf(stdout, "[Vulkan] - Available Extensions:\n");
				for (u32 i = 0; i < extensionCount; i++) {

					VkExtensionProperties* property = (extensions + i);
					std::fprintf(stdout, "\t%s\n", property->extensionName);
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

			std::fprintf(stderr, "[Vulkan] [Validation] - %s}\n", callbackData->pMessage);
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

	static bool VulkanCreateSurface(VulkanState* state, Window* window) {

		VkWin32SurfaceCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = glfwGetWin32Window(window->NativeHandle);
		createInfo.hinstance = GetModuleHandle(nullptr);

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

		if (capabilities->currentExtent.width != INFINITY) {

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

	static bool VulkanCreateSwapChain(VulkanState* state, Window* window) {

		VulkanSwapChainSupportDetails swapChainSupport = VulkanQuerySwapChainSupport(state, &state->PhysicalDevice);

		VkSurfaceFormatKHR surfaceFormat = VulkanChooseSwapSurfaceFormat(swapChainSupport.Formats, swapChainSupport.FormatCount);
		VkPresentModeKHR presentMode = VulkanChooseSwapPresentMode(swapChainSupport.PresentModes, swapChainSupport.PresentModeCount);
		VkExtent2D extent = VulkanChooseSwapExtent(&swapChainSupport.Capabilities, window);
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

		VkResult result = vkCreateSwapchainKHR(state->Device, &createInfo, nullptr, &state->SwapChain);
		if (result != VK_SUCCESS) {

			return false;
		}

		vkGetSwapchainImagesKHR(state->Device, state->SwapChain, &imageCount, nullptr);
		state->SwapChainImages = (VkImage*)malloc(imageCount * sizeof(VkImage));

		if (state->SwapChainImages) {

			vkGetSwapchainImagesKHR(state->Device, state->SwapChain, &imageCount, state->SwapChainImages);
			state->SwapChainImageFormat = surfaceFormat.format;
			state->SwapChainExtent = extent;
			state->SwapChainImageCount = imageCount;

			return true;
		}
		else {

			return false;
		}
	}

	static bool VulkanCreateImageViews(VulkanState* state) {

		state->SwapChainImageViews = (VkImageView*)malloc(state->SwapChainImageCount * sizeof(VkImageView));
		state->SwapChainImageViewCount = state->SwapChainImageCount;

		bool complete = true;
		for (u32 i = 0; i < state->SwapChainImageCount; i++) {

			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = *(state->SwapChainImages + i);
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = state->SwapChainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			VkImageView* imageView = (state->SwapChainImageViews + i);
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

			std::fprintf(stderr, "Couldn't create shader!");
		}

		return shaderModule;
	}

	static bool VulkanCreateRenderPass(VulkanState* state) {

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = state->SwapChainImageFormat;
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

		VulkanShaderCode vertexCode = VulkanLoadShaderCode("assets/handmade_triangle_vert.spv");
		VulkanShaderCode fragmentCode = VulkanLoadShaderCode("assets/handmade_triangle_frag.spv");

		VkShaderModule vertexShaderModule = VulkanCreateShaderModule(state, &vertexCode);
		VkShaderModule fragmentShaderModule = VulkanCreateShaderModule(state, &fragmentCode);

		VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
		vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderStageInfo.module = vertexShaderModule;
		vertexShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
		fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderStageInfo.module = fragmentShaderModule;
		fragmentShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[2] = { vertexShaderStageInfo, fragmentShaderStageInfo };

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (f32)state->SwapChainExtent.width;
		viewport.height = (f32)state->SwapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = state->SwapChainExtent;

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

			free(vertexCode.Data);
			free(fragmentCode.Data);
			vkDestroyShaderModule(state->Device, fragmentShaderModule, nullptr);
			vkDestroyShaderModule(state->Device, vertexShaderModule, nullptr);

			return pipelineResult == VK_SUCCESS;
		}
		else {

			free(vertexCode.Data);
			free(fragmentCode.Data);
			vkDestroyShaderModule(state->Device, fragmentShaderModule, nullptr);
			vkDestroyShaderModule(state->Device, vertexShaderModule, nullptr);
			return false;
		}
	}

	static bool VulkanCreateFramebuffers(VulkanState* state) {

		state->SwapChainFramebuffers = (VkFramebuffer*)malloc(state->SwapChainImageViewCount * sizeof(VkFramebuffer));
		state->SwapChainFramebufferCount = state->SwapChainImageViewCount;

		if (state->SwapChainFramebuffers) {

			bool complete = true;
			for (u32 i = 0; i < state->SwapChainImageViewCount; i++) {

				VkImageView* imageView = (state->SwapChainImageViews + i);
				VkFramebufferCreateInfo framebufferInfo{};
				framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebufferInfo.renderPass = state->Pipeline.RenderPass;
				framebufferInfo.attachmentCount = 1;
				framebufferInfo.pAttachments = imageView;
				framebufferInfo.width = state->SwapChainExtent.width;
				framebufferInfo.height = state->SwapChainExtent.height;
				framebufferInfo.layers = 1;

				if (vkCreateFramebuffer(state->Device, &framebufferInfo, nullptr, &state->SwapChainFramebuffers[i]) != VK_SUCCESS) {

					complete = false;
				}
			}

			return complete;
		}
		else {

			return false;
		}
	}

	static bool VulkanCreateCommandPool(VulkanState* state) {

		VulkanQueueFamilyIndices queueFamilyIndices = VulkanFindQueueFamilies(state, &state->PhysicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily;

		return vkCreateCommandPool(state->Device, &poolInfo, nullptr, &state->CommandPool) == VK_SUCCESS;
	}

	static bool VulkanCreateCommandBuffer(VulkanState* state) {

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = state->CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		return vkAllocateCommandBuffers(state->Device, &allocInfo, &state->CommandBuffer) == VK_SUCCESS;
	}

	static bool VulkanRecordCommandBuffer(VulkanState* state, VkCommandBuffer commandBuffer, u32 imageIndex) {

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		
		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {

			return false;
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = state->Pipeline.RenderPass;
		renderPassInfo.framebuffer = *(state->SwapChainFramebuffers + imageIndex);
		renderPassInfo.renderArea.offset = { 0 , 0 };
		renderPassInfo.renderArea.extent = state->SwapChainExtent;

		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->Pipeline.GraphicsPipeline);
			vkCmdDraw(commandBuffer, 3, 1, 0, 0);
		}
		vkCmdEndRenderPass(commandBuffer);

		return vkEndCommandBuffer(commandBuffer) == VK_SUCCESS;
	}

	static bool VulkanCreateSyncObjects(VulkanState* state) {

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VkResult imageAvailable = vkCreateSemaphore(state->Device, &semaphoreInfo, nullptr, &state->ImageAvailableSemaphore);
		VkResult renderFinished = vkCreateSemaphore(state->Device, &semaphoreInfo, nullptr, &state->RenderFinishedSemaphore);
		VkResult inFlight = vkCreateFence(state->Device, &fenceInfo, nullptr, &state->InFlightFence);

		return imageAvailable == VK_SUCCESS && renderFinished == VK_SUCCESS && inFlight == VK_SUCCESS;
	}

	bool VulkanStateInit(VulkanState* state, Window* window) {

		// Instance
		if (!VulkanCreateInstance(state)) {

			return false;
		}

		// Debug Messenger
		if (!VulkanCreateDebugMessenger(state)) {

			return false;
		}

		// Create Surface
		if (!VulkanCreateSurface(state, window)) {

			return false;
		}

		// Pick a Physical Device (GPU)
		if (!VulkanPickPhysicalDevice(state)) {

			return false;
		}

		// Create Logical Device
		if (!VulkanCreateLogicalDevice(state)) {

			return false;
		}

		// SwapChain
		if (!VulkanCreateSwapChain(state, window)) {

			return false;
		}

		// Image Views
		if (!VulkanCreateImageViews(state)) {

			return false;
		}

		// Render Pass
		if (!VulkanCreateRenderPass(state)) {

			return false;
		}

		// Graphics Pipeline
		if (!VulkanCreateGraphicsPipeline(state)) {

			return false;
		}

		// Framebuffers
		if (!VulkanCreateFramebuffers(state)) {

			return false;
		}

		// Command Pool
		if (!VulkanCreateCommandPool(state)) {

			return false;
		}

		// Command Buffer
		if (!VulkanCreateCommandBuffer(state)) {

			return false;
		}

		if (!VulkanCreateSyncObjects(state)) {

			return false;
		}

		return true;
	}

	bool VulkanStateDestroy(VulkanState* state) {

		vkDeviceWaitIdle(state->Device);

		vkDestroyCommandPool(state->Device, state->CommandPool, nullptr);

		// Sync Objects
		vkDestroySemaphore(state->Device, state->ImageAvailableSemaphore, nullptr);
		vkDestroySemaphore(state->Device, state->RenderFinishedSemaphore, nullptr);
		vkDestroyFence(state->Device, state->InFlightFence, nullptr);

		// Framebuffers
		for (u32 i = 0; i < state->SwapChainFramebufferCount; i++) {

			VkFramebuffer* framebuffer = (state->SwapChainFramebuffers + i);
			vkDestroyFramebuffer(state->Device, *framebuffer, nullptr);
		}
		free(state->SwapChainFramebuffers);

		// Pipeline
		vkDestroyPipeline(state->Device, state->Pipeline.GraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(state->Device, state->Pipeline.PipeLineLayout, nullptr);
		vkDestroyRenderPass(state->Device, state->Pipeline.RenderPass, nullptr);

		// Image Views
		for (u32 i = 0; i < state->SwapChainImageViewCount; i++) {

			VkImageView* imageView = (state->SwapChainImageViews + i);
			vkDestroyImageView(state->Device, *imageView, nullptr);
		}
		free(state->SwapChainImageViews);

		// SwapChain
		vkDestroySwapchainKHR(state->Device, state->SwapChain, nullptr);
		free(state->SwapChainImages);

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

	bool VulkanStateDrawFrame(VulkanState* state) {

		vkWaitForFences(state->Device, 1, &state->InFlightFence, VK_TRUE, UINT64_MAX);
		vkResetFences(state->Device, 1, &state->InFlightFence);

		u32 imageIndex{};
		vkAcquireNextImageKHR(state->Device, state->SwapChain, UINT64_MAX, state->ImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

		vkResetCommandBuffer(state->CommandBuffer, 0);
		VulkanRecordCommandBuffer(state, state->CommandBuffer, imageIndex);

		VkPipelineStageFlags waitStages[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &state->ImageAvailableSemaphore;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &state->CommandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &state->RenderFinishedSemaphore;

		if (vkQueueSubmit(state->GraphicsQueue, 1, &submitInfo, state->InFlightFence) != VK_SUCCESS) {

			return false;
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &state->RenderFinishedSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &state->SwapChain;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		vkQueuePresentKHR(state->PresentQueue, &presentInfo);

		return true;
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
