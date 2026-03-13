@echo off
REM Vulkan Engine - Add BOM to Code Files
REM This script adds UTF-8 BOM to all code files in the project

echo ========================================
echo Vulkan Engine - Add BOM Tool
echo ========================================
echo.

REM Check if Python is installed
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Python not found!
    echo Please install Python 3.6 or later.
    echo.
    pause
    exit /b 1
)

echo Python found.

REM Parse command line arguments
set MODE=ADD
if "%1"=="--check" set MODE=CHECK
if "%1"=="-c" set MODE=CHECK
if "%1"=="--remove" set MODE=REMOVE
if "%1"=="-r" set MODE=REMOVE
if "%1"=="--dry-run" set MODE=DRYRUN
if "%1"=="-d" set MODE=DRYRUN

echo.
echo Mode: %MODE%
echo.

REM Execute Python script
if "%MODE%"=="CHECK" (
    echo Checking which files need BOM...
    echo.
    python add_bom.py --check
) else if "%MODE%"=="REMOVE" (
    echo Removing BOM from code files...
    echo.
    python add_bom.py --remove --verbose
) else if "%MODE%"=="DRYRUN" (
    echo Dry run - showing what would be done...
    echo.
    python add_bom.py --dry-run
) else (
    echo Adding BOM to code files...
    echo.
    python add_bom.py --verbose
)

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Script failed with error code %errorlevel%
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo Operation completed successfully!
echo ========================================
echo.
pause
