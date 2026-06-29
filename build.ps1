# ============================================================================
# @file    build.ps1
# @brief   Synera PowerShell 构建脚本，自动查找工具链并编译项目
# @author  
# @date    2026-06-24
#
# 用法: .\build.ps1 [-Clean] [-Debug] [-Verbose]
# ============================================================================

param(
    [switch]$Clean,
    [switch]$Debug,
    [switch]$Verbose
)

# Set error handling
$ErrorActionPreference = "Continue"

# Console output functions
function Write-Header {
    param([string]$Text)
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "  $Text" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""
}

function Write-Success {
    param([string]$Text)
    Write-Host "[+] $Text" -ForegroundColor Green
}

function Write-Error {
    param([string]$Text)
    Write-Host "[X] $Text" -ForegroundColor Red
}

function Write-Info {
    param([string]$Text)
    Write-Host "[*] $Text" -ForegroundColor Yellow
}

# Main script
Write-Header "Synera: Synergy Auto-Arena - PowerShell Build"

# ========== Step 1：查找 CMake ==========
Write-Info "Finding CMake..."
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmake) {
    Write-Success "CMake found: $($cmake.Source)"
    $cmakeVersion = & cmake --version
    Write-Host $cmakeVersion[0] -ForegroundColor Gray
} else {
    Write-Error "CMake not found in PATH"
    
    # Try common installation paths
    $commonPaths = @(
        "C:\Program Files\CMake\bin\cmake.exe",
        "C:\Program Files (x86)\CMake\bin\cmake.exe",
        "D:\CMake\bin\cmake.exe"
    )
    
    $cmakePath = $null
    foreach ($path in $commonPaths) {
        if (Test-Path $path) {
            $cmakePath = $path
            Write-Success "Found CMake at: $path"
            break
        }
    }
    
    if (-not $cmakePath) {
        Write-Error "CMake installation not found!"
        Write-Host ""
        Write-Host "Please install CMake from: https://cmake.org/download/" -ForegroundColor Cyan
        Write-Host "Make sure to add CMake to your system PATH during installation"
        Write-Host ""
        exit 1
    }
}

# ========== Step 2：查找 Visual Studio ==========
Write-Info "Finding Visual Studio 2022..."
$vsPath = "C:\Program Files\Microsoft Visual Studio\2022\Community"
if (-not (Test-Path $vsPath)) {
    $vsPath = "C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
}
if (-not (Test-Path $vsPath)) {
    $vsPath = "C:\Program Files\Microsoft Visual Studio\2022\Professional"
}

if (Test-Path $vsPath) {
    Write-Success "Visual Studio 2022 found at: $vsPath"
} else {
    Write-Error "Visual Studio 2022 not found"
    Write-Host "Please install: https://visualstudio.microsoft.com/downloads/" -ForegroundColor Cyan
    exit 1
}

# ========== Step 3：检查 Qt ==========
Write-Info "Checking Qt 6.11.1..."
$qtPath = "C:\Qt\6.11.1\msvc2022_64"
if (Test-Path $qtPath) {
    Write-Success "Qt 6.11.1 found at: $qtPath"
} else {
    Write-Error "Qt 6.11.1 not found at default location"
    Write-Info "If Qt is installed elsewhere, update CMakeLists.txt with the correct path"
}

# ========== Step 4：准备构建目录 ==========
Write-Info "Setting up build directory..."
$buildDir = Join-Path (Get-Location) "build"
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
    Write-Success "Created build directory"
} else {
    if ($Clean) {
        Write-Info "Cleaning existing build directory..."
        Remove-Item $buildDir\* -Recurse -Force -ErrorAction SilentlyContinue
        Write-Success "Build directory cleaned"
    } else {
        Write-Info "Build directory already exists"
    }
}

# ========== Step 5：配置 CMake ==========
Write-Header "Configuring CMake"
Push-Location $buildDir

if ($Verbose) {
    Write-Host "CMake command: cmake -G `"Visual Studio 17 2022`" -DQt6_DIR=`"C:/Qt/6.11.1/msvc2022_64/lib/cmake/Qt6`" .." -ForegroundColor Gray
}

if ($cmakePath) {
    & $cmakePath -G "Visual Studio 17 2022" -DQt6_DIR="C:/Qt/6.11.1/msvc2022_64/lib/cmake/Qt6" ..
} else {
    & cmake -G "Visual Studio 17 2022" -DQt6_DIR="C:/Qt/6.11.1/msvc2022_64/lib/cmake/Qt6" ..
}

if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed!"
    Write-Host ""
    Write-Host "Possible reasons:" -ForegroundColor Cyan
    Write-Host "  1. Qt6_DIR path is incorrect"
    Write-Host "  2. Visual Studio 2022 is not properly installed"
    Write-Host "  3. CMake version is too old (need >= 3.20)"
    Write-Host ""
    Write-Host "For detailed troubleshooting, see: TROUBLESHOOTING.md" -ForegroundColor Cyan
    Pop-Location
    exit 1
}

Write-Success "CMake configuration completed"

# ========== Step 6：构建项目 ==========
Write-Header "Building Project"

$buildType = if ($Debug) { "Debug" } else { "Release" }
Write-Info "Build type: $buildType"

$buildCommand = "cmake --build . --config $buildType"
if ($Verbose) {
    $buildCommand += " -v"
}

Write-Info "Running: $buildCommand"
Invoke-Expression $buildCommand

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed!"
    Write-Host ""
    Write-Host "Please check the compilation errors above" -ForegroundColor Cyan
    Write-Host "For help, see: TROUBLESHOOTING.md" -ForegroundColor Cyan
    Pop-Location
    exit 1
}

Write-Success "Build completed successfully!"

# ========== Step 7：检查输出 ==========
$exePath = Join-Path $buildDir "bin\Synera.exe"
if (Test-Path $exePath) {
    Write-Success "Executable created: $exePath"
} else {
    Write-Error "Executable not found at expected location"
    Write-Info "Searching for output files..."
    Get-ChildItem $buildDir -Filter "*.exe" -Recurse | ForEach-Object {
        Write-Host "  Found: $($_.FullName)" -ForegroundColor Gray
    }
}

Pop-Location

# ========== Step 8：询问运行 ==========
Write-Header "Build Complete"

Write-Host "Executable location:" -ForegroundColor Cyan
Write-Host "  $exePath"
Write-Host ""

$response = Read-Host "Run program now? (y/n)"
if ($response.ToLower() -eq "y") {
    Write-Info "Launching game..."
    & $exePath
} else {
    Write-Info "You can run the program manually: $exePath"
}

Write-Host ""
