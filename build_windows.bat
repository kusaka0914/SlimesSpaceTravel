@echo off
setlocal EnableExtensions

cd /d "%~dp0"
if errorlevel 1 (
    echo [ERROR] Failed to open the project directory.
    exit /b 1
)

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
rem 2. Prepare a project-specific vcpkg installation
rem --------------------------------------------------

rem Use an isolated vcpkg to avoid changing another project's installation.
rem Keep dependency build paths outside deeply nested source directories.
set "VCPKG_BASELINE_COMMIT=664933b8d385dfc01f4e0cdff4c7c74e71f6a740"
set "VCPKG_CACHE_ROOT=%LOCALAPPDATA%\SlimesSpaceTravel"
set "VCPKG_ROOT=%VCPKG_CACHE_ROOT%\vcpkg"
set "VCPKG_INSTALLED_DIR=%VCPKG_CACHE_ROOT%\vcpkg_installed"

where git >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Git was not found.
    echo Please install Git for Windows and run this file again.
    pause
    exit /b 1
)

if not exist "%VCPKG_CACHE_ROOT%" (
    mkdir "%VCPKG_CACHE_ROOT%"
    if errorlevel 1 (
        echo [ERROR] Failed to create the vcpkg cache directory.
        pause
        exit /b 1
    )
)

if not exist "%VCPKG_ROOT%\.git" (
    if exist "%VCPKG_ROOT%" (
        echo [ERROR] An incomplete vcpkg installation was found:
        echo      %VCPKG_ROOT%
        echo Rename this directory and run this file again.
        pause
        exit /b 1
    )

    echo [INFO] Installing a project-specific vcpkg...
    git clone --no-checkout --depth 1 https://github.com/microsoft/vcpkg.git "%VCPKG_ROOT%"
    if errorlevel 1 (
        echo [ERROR] Failed to download vcpkg.
        pause
        exit /b 1
    )
)

rem The manifest baseline must be readable even in shallow Git clones.
git -C "%VCPKG_ROOT%" cat-file -e "%VCPKG_BASELINE_COMMIT%:versions/baseline.json" 2>nul
if errorlevel 1 (
    echo [INFO] Fetching the required vcpkg revision...
    git -C "%VCPKG_ROOT%" fetch --depth 1 origin "%VCPKG_BASELINE_COMMIT%"
    if errorlevel 1 (
        echo [ERROR] Failed to fetch the required vcpkg revision.
        pause
        exit /b 1
    )
)

git -C "%VCPKG_ROOT%" cat-file -e "%VCPKG_BASELINE_COMMIT%:versions/baseline.json" 2>nul
if errorlevel 1 (
    echo [ERROR] The required vcpkg baseline is unavailable.
    pause
    exit /b 1
)

git -C "%VCPKG_ROOT%" checkout --detach "%VCPKG_BASELINE_COMMIT%"
if errorlevel 1 (
    echo [ERROR] Failed to select the required vcpkg revision.
    pause
    exit /b 1
)

call "%VCPKG_ROOT%\bootstrap-vcpkg.bat" -disableMetrics
if errorlevel 1 (
    echo [ERROR] Failed to initialize vcpkg.
    pause
    exit /b 1
)

echo.
echo [OK] vcpkg ready:
echo      %VCPKG_ROOT%
echo [INFO] Dependency installation directory:
echo      %VCPKG_INSTALLED_DIR%
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
    -DCMAKE_TOOLCHAIN_FILE:FILEPATH="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
    -DVCPKG_TARGET_TRIPLET=x64-windows ^
    -DVCPKG_INSTALLED_DIR:PATH="%VCPKG_INSTALLED_DIR%"

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
