@echo off
rem Set environment variables
set VCPKG_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake
set BUILD_DIR=build
set BUILD_CONFIG=Debug

echo Vcpkg Toolchain: %VCPKG_TOOLCHAIN_FILE%
echo Build Directory: %BUILD_DIR%
echo Build Configuration: %BUILD_CONFIG%

rem Create build directory if it doesn't exist
if not exist "%BUILD_DIR%" (
    echo Creating build directory...
    mkdir "%BUILD_DIR%"
    if errorlevel 1 (
        echo Failed to create build directory.
        goto :error
    )
)

rem Change to build directory
cd "%BUILD_DIR%"
if errorlevel 1 (
    echo Failed to change to build directory.
    goto :error
)

rem Configure CMake
echo Configuring CMake...
cMAKE .. -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN_FILE%"
if errorlevel 1 (
    echo CMake configuration failed.
    cd ..
    goto :error
)

rem Build the project
echo Building project...
cMAKE --build . --config %BUILD_CONFIG%
if errorlevel 1 (
    echo Build failed.
    cd ..
    goto :error
)

echo Build completed successfully!

rem Run the built executable
rem cmd中支持中文
chcp 65001
echo Starting the application...
cd "%BUILD_CONFIG%"
start "" "WebStreamDeck.exe"

cd ..
goto :eof

:error
echo An error occurred.
pause
exit /b 1

:eof
pause 