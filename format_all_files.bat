@echo off

FOR /R "%~dp0\Source" %%f IN (*.cpp, *.h, *.cs) DO (
    clang-format -i "%%f"
    echo %%f
)

PAUSE