#include "handmade_window.h"

namespace handmade {

	static void WindowShouldCloseCallback(GLFWwindow* nativeHandle) {

		Window* window = (Window*)glfwGetWindowUserPointer(nativeHandle);
		window->Running = false;
	}

	bool WindowCreate(Window* window, const char* title, u32 width, u32 height) {

		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window->NativeHandle = glfwCreateWindow(width, height, title, nullptr, nullptr);
		window->Running = true;

		if (!window->NativeHandle) {

			std::fprintf(stderr, "Couldn't initialize window!\n");
			return false;
		}

		glfwSetWindowUserPointer(window->NativeHandle, window);
		glfwSetWindowCloseCallback(window->NativeHandle, WindowShouldCloseCallback);

		return true;
	}

	void WindowUpdate(Window* window) {

		glfwPollEvents();
	}

	void WindowDestroy(Window* window) {

		glfwDestroyWindow(window->NativeHandle);
		glfwTerminate();
	}
}