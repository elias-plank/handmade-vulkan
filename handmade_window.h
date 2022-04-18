/* date = April 12th 2022 11:38 pm */

#ifndef HANDMADE_WINDOW_H
#define HANDMADE_WINDOW_H

#include "handmade_types.h"

#pragma warning(disable : 26812)
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <cstdio>
#include <cassert>

namespace handmade {

	struct Window {

		GLFWwindow* NativeHandle;
		const char* Title;
		u32 Width;
		u32 Height;
		bool Running;
	};

	bool WindowCreate(Window* window, const char* title, u32 width, u32 height);
	void WindowUpdate(Window* window);
	void WindowDestroy(Window* window);
}

#endif //HANDMADE_WINDOW_H
