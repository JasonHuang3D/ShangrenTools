@echo off

:: force to use UTF-8 code page
chcp 65001

pushd %~dp0

:: Configs
set target_name=TianyuanCalcCMD
set build_folder=build
set build_mode=MinSizeRel
set dest_folder=%~dp0bin\
set dest_name=通用副本工具beta1.3.zip
set intro_file="说明.txt"

set parent_folder=%~dp0..\
set parent_build_folder=%parent_folder%%build_folder%\
set bin_folder=%parent_build_folder%Code\%target_name%\%build_mode%\
set project_folder=%parent_folder%Code\%target_name%\

set dest_file=%dest_folder%%dest_name%
if exist "%dest_file%" (
del %dest_file%
)

if not exist "%dest_folder%" (
mkdir %dest_folder%
)


set target_exe=%bin_folder%%target_name%.exe
set target_input_file=%project_folder%inputData.txt
set target_target_file=%project_folder%targetData.txt

if not exist "%target_exe%" (
cd %parent_folder%
call cmake -B %parent_build_folder%
call cmake --build %parent_build_folder% --config MINSIZEREL
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


