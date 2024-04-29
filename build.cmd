@echo off

pushd .

cd win

set dirbuild=.\build
set dirbin=.\bin
set dirsrc=..\src

echo Removing old build files
rmdir /S /Q %dirbuild% %dirbin%

echo Creating new build directories
mkdir %dirbuild%
mkdir %dirbin%

xcopy .\dlls\*.dll %dirbin%

set srcfiles=%dirsrc%\main.c

set opts=/O2 /TC /D "_CRT_SECURE_NO_WARNINGS" /nologo /Fobuild\
set includes=/I.\include /I%dirsrc% /I.\dlls
set libs="OpenGL32.lib" "GLu32.lib" "glfw3_mt.lib" "glew32.lib" "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib"

echo Compiling project
set cmd=cl %opts% %includes% %srcfiles% /link /LIBPATH:lib /NODEFAULTLIB:MSVCRT %libs% /OUT:bin\cgui.exe 
echo %cmd%
%cmd%

popd
