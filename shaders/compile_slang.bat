@echo off
REM Compile Slang shaders to SPIR-V for Vulkan

echo Compiling Slang shaders...

REM Compile triangle shaders
echo Compiling triangle shaders...
slangc -target spirv -stage vertex -entry vertexMain triangle.slang -o triangle.vert.spv
if %ERRORLEVEL% NEQ 0 (
    echo Failed to compile triangle vertex shader!
    exit /b 1
)
echo Vertex shader compiled: triangle.vert.spv

slangc -target spirv -stage fragment -entry fragmentMain triangle.slang -o triangle.frag.spv
if %ERRORLEVEL% NEQ 0 (
    echo Failed to compile triangle fragment shader!
    exit /b 1
)
echo Fragment shader compiled: triangle.frag.spv

REM Compile PBR shaders
echo Compiling PBR shaders...
slangc -target spirv -stage vertex -entry vertexMain pbr.slang -o pbr.vert.spv
if %ERRORLEVEL% NEQ 0 (
    echo Failed to compile PBR vertex shader!
    exit /b 1
)
echo PBR Vertex shader compiled: pbr.vert.spv

slangc -target spirv -stage fragment -entry fragmentMain pbr.slang -o pbr.frag.spv
if %ERRORLEVEL% NEQ 0 (
    echo Failed to compile PBR fragment shader!
    exit /b 1
)
echo PBR Fragment shader compiled: pbr.frag.spv

echo.
echo All shaders compiled successfully!
pause
