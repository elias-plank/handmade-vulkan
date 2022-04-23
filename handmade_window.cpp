#include "handmade_window.h"

namespace handmade {

	bool WindowCreate(Window* window, const char* title, u32 width, u32 height) {

		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window->NativeHandle = glfwCreateWindow(width, height, title, nullptr, nullptr);
		window->Running = true;

		if (!window->NativeHandle) {

			std::fprintf(stderr, "Couldn't initialize window!\n");
			return false;
		}

		return true;
	}

	bool WindowIsRunning(Window* window) {

		return window->Running && !glfwWindowShouldClose(window->NativeHandle);
	}

	void WindowUpdate(Window* window) {

		glfwPollEvents();
	}

	void WindowDestroy(Window* window) {

		glfwDestroyWindow(window->NativeHandle);
		glfwTerminate();
	}
}