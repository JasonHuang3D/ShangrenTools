@echo off

pushd %~dp0

set build_folder="build"

if exist %build_folder% (
rmdir /s /q %build_folder%
)
popd

pause