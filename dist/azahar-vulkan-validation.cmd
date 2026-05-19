@echo off
setlocal
set VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation
set QT_FATAL_WARNINGS=0
start "" "%~dp0azahar.exe"
endlocal
