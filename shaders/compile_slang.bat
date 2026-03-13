@echo off
REM Compile Slang shaders to SPIR-V for Vulkan

echo Compiling Slang shaders...

REM Compile vertex shader
slangc -target spirv -stage vertex -entry vertexMain triangle.slang -o triangle.vert.spv
if %ERRORLEVEL% NEQ 0 (
    echo Failed to compile vertex shader!
    exit /b 1
)
echo Vertex shader compiled: triangle.vert.spv

REM Compile fragment shader
slangc -target spirv -stage fragment -entry fragmentMain triangle.slang -o triangle.frag.spv
if %ERRORLEVEL% NEQ 0 (
    echo Failed to compile fragment shader!
    exit /b 1
)
echo Fragment shader compiled: triangle.frag.spv

echo.
echo All shaders compiled successfully!
pause
