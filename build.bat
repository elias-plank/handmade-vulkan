if exist build\ (
	echo Detected build folder
) else (
	mkdir build
)
cd build
cmake ..
cmake --build .