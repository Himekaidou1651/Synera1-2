@echo off
chcp 65001 >nul
title Synera2026

REM ============================================================================
REM @file    run.bat
REM @brief   Synera 一键运行脚本，自动检查环境、构建并启动游戏
REM @author  
REM @date    2026-06-24
REM ============================================================================

setlocal enabledelayedexpansion

set PASS=[+]
set FAIL=[X]
set INFO=[*]

echo ========================================
echo  Synera2026 - Launcher
echo ========================================
echo.

echo %INFO% Checking development environment...
echo.

REM ========== 1. 检查 CMake ==========
echo %INFO% Checking CMake...
set CMAKE_OK=0
where cmake >nul 2>&1
if %errorlevel% equ 0 (
  echo %PASS% CMake found in PATH
  set CMAKE_OK=1
  goto :cmake_done
)
if exist "C:\Progra~1\CMake\bin\cmake.exe" (
  echo %PASS% CMake found
  set "PATH=C:\Progra~1\CMake\bin;%PATH%"
  set CMAKE_OK=1
  goto :cmake_done
)
if exist "C:\Progra~2\CMake\bin\cmake.exe" (
  echo %PASS% CMake found
  set "PATH=C:\Progra~2\CMake\bin;%PATH%"
  set CMAKE_OK=1
  goto :cmake_done
)
echo %FAIL% CMake NOT found
:cmake_done
echo.

REM ========== 2. 检查 Qt ==========
echo %INFO% Checking Qt...
set QT_OK=0
set QT_DIR=
if exist "D:\Qt\6.11.1\mingw_64" (
  echo %PASS% Qt 6.11.1 found
  set QT_OK=1
  set QT_DIR=D:\Qt\6.11.1\mingw_64
  goto :qt_done
)
if exist "C:\Qt\6.11.1\msvc2022_64" (
  echo %PASS% Qt 6.11.1 found
  set QT_OK=1
  set QT_DIR=C:\Qt\6.11.1\msvc2022_64
  goto :qt_done
)
echo %FAIL% Qt 6.11.1 NOT found
:qt_done
echo.

REM ========== 3. 检查项目文件 ==========
echo %INFO% Checking project files...
if exist "CMakeLists.txt" (echo %PASS% CMakeLists.txt found) else (echo %FAIL% CMakeLists.txt NOT found)
if exist "src\main.cpp" (echo %PASS% src/main.cpp found) else (echo %FAIL% src/main.cpp NOT found)
echo.

REM ========== 4. 查找或构建可执行文件 ==========
echo %INFO% Looking for executable...
set EXE_PATH=
if exist "build\Desktop_Qt_6_11_1_MinGW_64_bit_Debug\bin\Synera.exe" set EXE_PATH=build\Desktop_Qt_6_11_1_MinGW_64_bit_Debug\bin\Synera.exe
if exist "build\bin\Synera.exe" set EXE_PATH=build\bin\Synera.exe

if defined EXE_PATH (
  echo %PASS% Found: %EXE_PATH%
  echo.
  goto :deploy
)

echo %INFO% Executable not found, starting build...
echo.

if %CMAKE_OK% equ 0 (
  echo %FAIL% Cannot build without CMake
  pause
  exit /b 1
)
if %QT_OK% equ 0 (
  echo %FAIL% Cannot build without Qt
  pause
  exit /b 1
)

if not exist build mkdir build
cd build

echo %INFO% Configuring CMake...
cmake -G "MinGW Makefiles" -DQt6_DIR="%QT_DIR%/lib/cmake/Qt6" ..
if errorlevel 1 (
  echo %FAIL% CMake configuration failed!
  pause
  exit /b 1
)

echo %INFO% Building...
cmake --build . --config Release
if errorlevel 1 (
  echo %FAIL% Build failed!
  pause
  exit /b 1
)

cd ..
echo %PASS% Build successful!
echo.

if exist "build\Desktop_Qt_6_11_1_MinGW_64_bit_Debug\bin\Synera.exe" set EXE_PATH=build\Desktop_Qt_6_11_1_MinGW_64_bit_Debug\bin\Synera.exe
if exist "build\bin\Synera.exe" set EXE_PATH=build\bin\Synera.exe

if not defined EXE_PATH (
  echo %FAIL% Executable not found after build!
  pause
  exit /b 1
)

:deploy
REM ========== 5. 部署 Qt 动态库 ==========
echo %INFO% Deploying Qt libraries...

set WINDEPLOYQT=D:\Qt\6.11.1\mingw_64\bin\windeployqt.exe
if not exist "%WINDEPLOYQT%" set WINDEPLOYQT=C:\Qt\6.11.1\mingw_64\bin\windeployqt.exe

if exist "%WINDEPLOYQT%" (
  for %%I in ("%EXE_PATH%") do set EXE_DIR=%%~dpI
  "%WINDEPLOYQT%" --dir "%EXE_DIR%" "%EXE_PATH%" >nul 2>&1
  echo %PASS% Qt libraries deployed
) else (
  echo %INFO% windeployqt not found, skipping DLL deployment
)

REM ========== 6. 启动游戏 ==========
echo.
echo %INFO% Launching Synera2026...
start "" "%EXE_PATH%"
echo %PASS% Game launched!
echo.

endlocal
