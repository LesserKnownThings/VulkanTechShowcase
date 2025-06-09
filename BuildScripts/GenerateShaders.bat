@echo off
setlocal

set "folder=..\ShadersGLSL"

rem Compile normal shaders (vert, frag)
for /d %%F in ("%folder%\*") do (
    if /i not "%%~F"=="%folder%\Include" (
		if not exist "..\Data\Engine\Shaders\%%~nxF" (
			mkdir "..\Data\Engine\Shaders\%%~nxF"
		)
		
		echo Compiling %%~nxF
		
        glslc "%%F\shader.vert" -o "..\Data\Engine\Shaders\%%~nxF\vert.spv"
		
		rem All other stages might not exist so I need to check first
		if exist "%%F\shader.frag" (
			glslc "%%F\shader.frag" -o "..\Data\Engine\Shaders\%%~nxF\frag.spv"
		) else ( 
			echo Missing fragment stage, skipping stage!
		)
		
		if exist "%%F\shader.geom" (
			glslc "%%F\shader.geom" -o "..\Data\Engine\Shaders\%%~nxF\geom.spv"
		) else ( 
			echo Missing geometry stage, skipping stage!
		)
		
		echo ****************
    )
)

echo Successfully compiled all shaders

endlocal

pause