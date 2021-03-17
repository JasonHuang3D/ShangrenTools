@echo off

:: force to use UTF-8 code page
chcp 65001

pushd %~dp0

:: Configs
set target_name=GearCalcCMD
set build_folder=build
set build_mode=Release
set dest_folder=%~dp0bin\
set dest_name=仙界模拟器beta1.2.0.zip
set intro_file="README.txt"
set additional_files=(XianjieData.json XianqiData.json)

set root_folder=%~dp0..\..\
set root_build_folder=%root_folder%%build_folder%\
set bin_folder=%root_build_folder%Code\Tools\%target_name%\%build_mode%\
set project_folder=%root_folder%Code\Tools\%target_name%\
set dest_file=%dest_folder%%dest_name%
set tools_folder=%root_folder%Tools\

if exist "%dest_folder%" (
rmdir /s /q %dest_folder%
) else (
mkdir %dest_folder%
)

cd %root_folder%
call cmake -B %root_build_folder%
call cmake --build %root_build_folder% --config %build_mode%
cd %~dp0

call :pack_files
pause
popd
exit /b %errorlevel%

:pack_files
setlocal enabledelayedexpansion
set target_exe=%bin_folder%%target_name%.exe
for %%a in %additional_files% do (
    set flat_additional_files=!flat_additional_files! %project_folder%%%a
)
call %tools_folder%7z.exe a %dest_file% %target_exe% %flat_additional_files% %intro_file%
endlocal
exit /b %errorlevel%


