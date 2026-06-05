@echo off
REM Build script for marquee-console using MinGW-w64 g++
REM Requires: g++ (from MSYS2, Cygwin, or similar)

echo Building marquee-console...
echo.

g++ -std=c++17 -Wall -Wextra -Wpedantic -c src\engine.cpp -o engine.o
if %errorlevel% neq 0 exit /b %errorlevel%

g++ -std=c++17 -Wall -Wextra -Wpedantic -c src\calibrate.cpp -o calibrate.o
if %errorlevel% neq 0 exit /b %errorlevel%

g++ -std=c++17 -Wall -Wextra -Wpedantic -c src\marquee.cpp -o marquee.o
if %errorlevel% neq 0 exit /b %errorlevel%

g++ -std=c++17 -Wall -Wextra -Wpedantic -c src\main.cpp -o main.o
if %errorlevel% neq 0 exit /b %errorlevel%

g++ -std=c++17 engine.o calibrate.o marquee.o main.o -o marquee-console.exe -static
if %errorlevel% neq 0 exit /b %errorlevel%

echo.
echo Building test_marquee...
g++ -std=c++17 -Wall -Wextra -Wpedantic src\test_marquee.cpp src\engine.cpp src\calibrate.cpp src\marquee.cpp -o test_marquee.exe -static
if %errorlevel% neq 0 exit /b %errorlevel%

echo.
echo Build successful: marquee-console.exe + test_marquee.exe
