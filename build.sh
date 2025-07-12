#!/bin/sh

mkdir -p build

cd external
./prepare_dependencies.sh
cd ../build
cmake ../src --preset "desktop-distro-linux" -DBUILD_SHARED_LIBS=OFF $1
cd distro
cmake --build . --parallel

