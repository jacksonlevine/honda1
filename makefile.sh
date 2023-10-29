#!/bin/bash

mkdir build

cd build

cmake -DCMAKE_TOOLCHAIN_FILE=~/Documents/vcpkg/scripts/buildsystems/vcpkg.cmake ..

cmake --build . --config Release || {
    echo "Build failed."
    exit 1
}

cp -f Release/main.exe ..
cp -f Release/glew32.dll ..
cp -f Release/glfw3.dll ..

echo "Build and file copying completed successfully."
