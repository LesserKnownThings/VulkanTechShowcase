@echo off
setlocal

set "folder=..\ShadersGLSL"

rem Compile normal shaders (vert, frag)
for /d %%F in ("%folder%\*") do (
    if /i not "%%~F"=="%folder%\Include" (
		if not exist "..\Data\Engine\Shaders\%%~nxF" (
			mkdir "..\Data\Engine\Shaders\%%~nxF"
		)
        glslc "%%F\shader.vert" -o "..\Data\Engine\Shaders\%%~nxF\vert.spv"
        glslc "%%F\shader.frag" -o "..\Data\Engine\Shaders\%%~nxF\frag.spv"		
    )
)

echo Successfully compiled all shaders

endlocal

pause