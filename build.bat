@echo off
REM ============================================================
REM  build.bat - Build nhanh bang g++ (MSYS2 ucrt64)
REM  Khong can CMake, khong can thu vien ngoai
REM ============================================================
setlocal

set CXX=C:\msys64\ucrt64\bin\g++.exe
set SRC=app\main.cpp
set OUT=parking_gui.exe
set INC=-I.\app -I.\lib
set FLAGS=-std=c++17 -O2 -DUNICODE -D_UNICODE %INC%
set LIBS=-lgdi32 -lcomctl32 -lcomdlg32 -mwindows -static -static-libgcc -static-libstdc++

echo [BUILD] He thong Quan ly Nha xe - GUI Win32
echo.

if not exist "%CXX%" (
    echo [LOI] Khong tim thay g++ tai: %CXX%
    echo       Vui long cai MSYS2 va ucrt64 toolchain.
    pause & exit /b 1
)

echo [1/2] Dang compile...
"%CXX%" %FLAGS% -o %OUT% %SRC% %LIBS%
if errorlevel 1 (
    echo [LOI] Compile that bai!
    pause & exit /b 1
)

echo [2/2] Sao chep file du lieu...
if exist map.txt         copy /Y map.txt map.txt >nul

echo.
echo [OK] Build thanh cong!
echo [OK] Chay: %OUT%
echo.
start "" "%OUT%"
