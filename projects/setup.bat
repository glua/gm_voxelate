@echo off
echo Generating Windows project...
..\..\..\premake5 --os=windows vs2013
echo.
echo Generating Mac project...
..\..\..\premake5 --os=macosx gmake
echo.
echo Generating Linux project...
..\..\..\premake5 --os=linux gmake
echo.
pause