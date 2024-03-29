# handmade-vulkan

Vulkan Programs written in C++ with no STL.

![vulkan](https://github.com/elias-plank/handmade-vulkan/assets/45200003/12b63597-e813-4dd0-8c72-6af010cb1cdc)

## Requirements
- [CMake](https://cmake.org)
- [Windows 10/11](https://www.microsoft.com/software-download/windows11)
- [Visual Studio 2022](https://visualstudio.com) (not strictly required, however it is my prefered development environment)
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#windows) (preferably a recent version)
- [vcpkg](https://github.com/microsoft/vcpkg) (package manager for glfw, planning to remove this in the future)

## Getting Started
Once you've cloned, change the *CMAKE_PREFIX_PATH* variable in the **CMakeLists.txt** file to match your vcpkg install. Assuming your cmake installation is already configured, you can run the **build.bat** file to build the project. The binaries as well as the required assets should be placed into a folder called **binaries/**. 

### 3rd party libaries
- [GLFW](https://github.com/glfw/glfw) (planning to remove this in the future)
