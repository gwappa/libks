@echo off
cd /d %~dp0
cl /c /O2 /Wall /EHsc /Iinclude src\ks\*.cpp
lib /OUT:libks.lib *.obj
del *.obj
exit /b 0
