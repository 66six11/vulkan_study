@echo off
REM Vulkan Engine - Quick Build Script
REM This script calls the actual build script in tools/

call "%~dp0tools\build.bat" %*
