#!/bin/bash

mkdir build

cd build

cmake -DCMAKE_TOOLCHAIN_FILE=~/Documents/vcpkg/scripts/buildsystems/vcpkg.cmake ..

cmake --build . --config arm64-osx-rel || {
    echo "Build failed."
    exit 1
}

cp -f main ..

echo "Build and file copying completed successfully."
