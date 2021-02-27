@echo off

:: force to use UTF-8 code page
chcp 65001

pushd %~dp0

:: Configs
set target_name=TianyuanCalcCMD
set build_folder=build
set build_mode=Release
set dest_folder=%~dp0bin\
set dest_name=通用副本工具beta1.7.zip
set intro_file="更新及说明.txt"

set root_folder=%~dp0..\..\
set root_build_folder=%root_folder%%build_folder%\
set bin_folder=%root_build_folder%Code\Tools\%target_name%\%build_mode%\
set project_folder=%root_folder%Code\Tools\%target_name%\
set dest_file=%dest_folder%%dest_name%

if exist "%dest_folder%" (
rmdir /s /q %dest_folder%
) else (
mkdir %dest_folder%
)


set target_exe=%bin_folder%%target_name%.exe
set target_input_file=%project_folder%inputData.txt
set target_target_file=%project_folder%targetData.txt

if not exist "%target_exe%" (
cd %root_folder%
call cmake -B %root_build_folder%
call cmake --build %root_build_folder% --config %build_mode%
cd %~dp0
)

call :pack_files

pause
popd
exit /b %errorlevel%


:pack_files
setlocal
call %~dp07z.exe a %dest_file% %target_exe% %target_input_file% %target_target_file% %intro_file%
endlocal
exit /b %errorlevel%


