@echo off
setlocal

if not exist build (
    mkdir build
)

cd external
call prepare_dependencies.bat

cd ../build
cmake ../src --preset "desktop-distro-windows" -DBUILD_SHARED_LIBS=OFF %1

cd distro
cmake --build . --config Release --parallel

endlocal

