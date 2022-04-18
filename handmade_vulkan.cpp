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
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
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

			return true;
		}
		else {

			return false;
		}
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

		return true;
	}

	bool VulkanStateDestroy(VulkanState* state) {

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
}