@echo off
setlocal EnableExtensions

cd /d "%~dp0"

echo ==========================================
echo  Slime's Space Travel - Windows Build
echo ==========================================
echo.

rem --------------------------------------------------
rem 1. Find CMake
rem --------------------------------------------------

set "CMAKE_EXE="

where cmake >nul 2>nul
if %errorlevel%==0 (
    for /f "delims=" %%i in ('where cmake') do (
        if not defined CMAKE_EXE set "CMAKE_EXE=%%i"
    )
)

rem Keep discovery in a subroutine so values are expanded after assignment.
if not defined CMAKE_EXE call :find_visual_studio_cmake

if not defined CMAKE_EXE (
    echo [ERROR] CMake was not found.
    echo.
    echo Please install Visual Studio 2022 with:
    echo   Desktop development with C++
    echo.
    if /i "%~1"=="--check-cmake" exit /b 1
    pause
    exit /b 1
)

echo [OK] CMake:
echo      %CMAKE_EXE%
echo.
if /i "%~1"=="--check-cmake" exit /b 0

rem --------------------------------------------------
rem 2. Find / Install vcpkg
rem --------------------------------------------------

if defined VCPKG_ROOT (
    if exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
        echo [OK] Using existing vcpkg:
        echo      %VCPKG_ROOT%
        goto vcpkg_ready
    )
)

set "VCPKG_ROOT=%LOCALAPPDATA%\SlimesSpaceTravel\vcpkg"

if exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    echo [OK] Using cached vcpkg:
    echo      %VCPKG_ROOT%
    goto vcpkg_ready
)

echo [INFO] vcpkg was not found.
echo [INFO] Installing vcpkg automatically...
echo.

where git >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Git was not found.
    echo.
    echo Please install Git for Windows and run this file again.
    pause
    exit /b 1
)

if not exist "%LOCALAPPDATA%\SlimesSpaceTravel" (
    mkdir "%LOCALAPPDATA%\SlimesSpaceTravel"
)

git clone --no-checkout https://github.com/microsoft/vcpkg.git "%VCPKG_ROOT%"
if errorlevel 1 (
    echo.
    echo [ERROR] Failed to download vcpkg.
    pause
    exit /b 1
)

rem Match the tool revision used by CI; builtin-baseline pins the dependency ports.
git -C "%VCPKG_ROOT%" checkout --detach 664933b8d385dfc01f4e0cdff4c7c74e71f6a740
if errorlevel 1 (
    echo [ERROR] Failed to select the supported vcpkg revision.
    pause
    exit /b 1
)

call "%VCPKG_ROOT%\bootstrap-vcpkg.bat" -disableMetrics
if errorlevel 1 (
    echo.
    echo [ERROR] Failed to initialize vcpkg.
    pause
    exit /b 1
)

:vcpkg_ready

echo.
echo [OK] vcpkg ready.
echo.

rem --------------------------------------------------
rem 3. Configure
rem --------------------------------------------------

set "BUILD_DIR=%~dp0out\build\windows-x64-release"

echo ==========================================
echo  Configuring project
echo ==========================================
echo.

"%CMAKE_EXE%" ^
    -S . ^
    -B "%BUILD_DIR%" ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
    -DVCPKG_TARGET_TRIPLET=x64-windows

if errorlevel 1 (
    echo.
    echo ==========================================
    echo  CONFIGURE FAILED
    echo ==========================================
    echo.
    pause
    exit /b 1
)

rem --------------------------------------------------
rem 4. Build game
rem --------------------------------------------------

echo.
echo ==========================================
echo  Building Release
echo ==========================================
echo.

"%CMAKE_EXE%" ^
    --build "%BUILD_DIR%" ^
    --config Release ^
    --target game ^
    --parallel

if errorlevel 1 (
    echo.
    echo ==========================================
    echo  BUILD FAILED
    echo ==========================================
    echo.
    pause
    exit /b 1
)

echo.
echo ==========================================
echo  BUILD SUCCESSFUL
echo ==========================================
echo.
echo Build directory:
echo %BUILD_DIR%
echo.
pause

exit /b 0

:find_visual_studio_cmake
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VS_PATH="
if not exist "%VSWHERE%" exit /b 0
for /f "usebackq tokens=*" %%i in (
    `"%VSWHERE%" -latest -products * -property installationPath`
) do (
    set "VS_PATH=%%i"
)
if not defined VS_PATH exit /b 0
if exist "%VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
    set "CMAKE_EXE=%VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
)
exit /b 0
