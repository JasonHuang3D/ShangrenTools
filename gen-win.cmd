@echo off

pushd %~dp0
call cmake -B ./build
popd

pause