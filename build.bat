@echo off
REM Atlas Tune Pro - build script (Windows, Visual Studio 2022 required)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 goto :err
cmake --build build --config Release --target AtlasTunePro_VST3
if errorlevel 1 goto :err
echo.
echo Done. Plugin: build\AtlasTunePro_artefacts\Release\VST3\Atlas Tune Pro.vst3
echo Copy that folder to C:\Program Files\Common Files\VST3\  (run Explorer as Administrator)
goto :eof
:err
echo BUILD FAILED
exit /b 1
