@echo off
call build.cmd
pushd .
cd win\bin
cgui.exe
popd
