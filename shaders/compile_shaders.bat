:: I got AI (claude 3.7 sonnet) to generate this shader compile script because I genuinely could not be bothered.

@echo off
setlocal enabledelayedexpansion

set GLSLC_PATH=C:\VulkanSDK\1.3.290.0\Bin\glslangValidator.exe

echo Vulkan Shader Compiler Batch Script
echo -----------------------------------
echo.

:: Check if the compiler exists
if not exist "%GLSLC_PATH%" (
    echo Error: Vulkan shader compiler not found at %GLSLC_PATH%
    echo Please verify your Vulkan SDK installation path.
    exit /b 1
)

echo Found glslc compiler at %GLSLC_PATH%
echo Starting shader compilation...
echo.

set VERT_COUNT=0
set FRAG_COUNT=0
set COMP_COUNT=0
set GLSL_COUNT=0
set ERROR_COUNT=0

:: Compile all vertex shaders
for %%f in (*.vert) do (
    echo Compiling vertex shader: %%f
    "%GLSLC_PATH%" -V "%%f" -o "%%f.spv" -gVS
    if !errorlevel! neq 0 (
        echo Error compiling %%f
        set /a ERROR_COUNT+=1
    ) else (
        echo Successfully compiled %%f to %%f.spv
        set /a VERT_COUNT+=1
    )
    echo.
)

:: Compile all fragment shaders
for %%f in (*.frag) do (
    echo Compiling fragment shader: %%f
    "%GLSLC_PATH%" -V "%%f" -o "%%f.spv" -gVS
    if !errorlevel! neq 0 (
        echo Error compiling %%f
        set /a ERROR_COUNT+=1
    ) else (
        echo Successfully compiled %%f to %%f.spv
        set /a FRAG_COUNT+=1
    )
    echo.
)

:: Compile all compute shaders
for %%f in (*.comp) do (
    echo Compiling compute shader: %%f
    "%GLSLC_PATH%" -V "%%f" -o "%%f.spv" -gVS
    if !errorlevel! neq 0 (
        echo Error compiling %%f
        set /a ERROR_COUNT+=1
    ) else (
        echo Successfully compiled %%f to %%f.spv
        set /a COMP_COUNT+=1
    )
    echo.
)


echo Compilation complete!
echo -----------------------------------
echo Vertex shaders compiled: %VERT_COUNT%
echo Fragment shaders compiled: %FRAG_COUNT%
echo Compute shaders compiled: %COMP_COUNT%
set /a TOTAL = VERT_COUNT + FRAG_COUNT + COMP_COUNT
echo Total shaders compiled: %TOTAL%
echo Errors encountered: %ERROR_COUNT%

if %ERROR_COUNT% gtr 0 (
    echo Some shaders failed to compile. Please check the output above.
) else if %TOTAL% equ 0 (
    echo No shader files found in the current directory.
) else (
    echo All shaders compiled successfully!
)
