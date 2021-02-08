@echo off
chcp 65001

pushd %~dp0

:: Configs
set target_name=TianyuanCalcCMD
set build_folder=build
set build_mode=MinSizeRel
set dest_name=通用副本工具beta1.2.zip
set intro_file="说明.txt"

set parent_folder=%~dp0../
set bin_folder=%parent_folder%%build_folder%/%target_name%/%build_mode%/
set project_folder=%parent_folder%%target_name%/

set dest_file=%~dp0%dest_name%
if exist "%dest_file%" del %dest_file%

set target_exe=%bin_folder%%target_name%.exe
set target_input_file=%project_folder%inputData.txt
set target_target_file=%project_folder%targetData.txt

call :pack_files

pause
popd
exit /b %errorlevel%


:pack_files
setlocal
call %~dp07z.exe a %dest_name% %target_exe% %target_input_file% %target_target_file% %intro_file%
endlocal
exit /b %errorlevel%


