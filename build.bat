if exist build\ (
	echo Detected build folder
) else (
	mkdir build
)
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .