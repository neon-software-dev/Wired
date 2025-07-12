@echo off
setlocal

REM Create build directory if it doesn't exist
if not exist build (
    mkdir build
)

REM Run dependency preparation script
cd external
call prepare_dependencies.bat
cd ..

REM Run CMake with preset and option
cd build
cmake ../src --preset "desktop-distro-windows" -DBUILD_SHARED_LIBS=OFF %1

REM Build using MSBuild
cd distro
cmake --build . --config Release --parallel

endlocal

