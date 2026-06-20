@echo off
setlocal EnableExtensions
chcp 65001 > nul

set "PROJECT_DIR=%~dp0"
cd /d "%PROJECT_DIR%" || exit /b 1

set "COMMAND=%~1"
if "%COMMAND%"=="" set "COMMAND=all"

if exist "C:\msys64\ucrt64\bin\g++.exe" (
    set "CXX=C:\msys64\ucrt64\bin\g++.exe"
) else (
    where g++ > nul 2> nul
    if errorlevel 1 (
        echo [LOI] Khong tim thay g++. Cai MSYS2 UCRT64 hoac them g++ vao PATH.
        exit /b 1
    )
    set "CXX=g++"
)

set "INCLUDES=-I.\app -I.\lib"
set "COMMON=-std=c++17 -O2 -Wall -Wextra -Wpedantic -finput-charset=UTF-8 -fexec-charset=UTF-8"
set "DEBUG=-std=c++17 -O0 -g3 -Wall -Wextra -Wpedantic -finput-charset=UTF-8 -fexec-charset=UTF-8"
set "TUI_OUT=parking_tui.exe"
set "GUI_OUT=parking_gui.exe"
set "TEST_OUT=backend_tests.exe"

echo [INFO] Project : %PROJECT_DIR%
echo [INFO] Compiler: %CXX%
echo [INFO] Command : %COMMAND%
echo.

if /I "%COMMAND%"=="all" goto build_all
if /I "%COMMAND%"=="tui" goto build_tui
if /I "%COMMAND%"=="gui" goto build_gui
if /I "%COMMAND%"=="clean" goto clean
if /I "%COMMAND%"=="run-tui" goto run_tui
if /I "%COMMAND%"=="run-gui" goto run_gui
if /I "%COMMAND%"=="debug" goto debug
if /I "%COMMAND%"=="test" goto test

echo [LOI] Lenh khong hop le: %COMMAND%
echo       Dung: build.bat [all^|tui^|gui^|clean^|run-tui^|run-gui^|debug^|test]
exit /b 1

:build_all
call :build_tui_impl "%COMMON%" || exit /b 1
call :build_gui_impl "%COMMON%" || exit /b 1
echo [OK] Build all thanh cong.
exit /b 0

:build_tui
call :build_tui_impl "%COMMON%" || exit /b 1
exit /b 0

:build_gui
call :build_gui_impl "%COMMON%" || exit /b 1
exit /b 0

:debug
call :clean_impl
call :build_tui_impl "%DEBUG%" || exit /b 1
call :build_gui_impl "%DEBUG%" || exit /b 1
echo [OK] Build debug thanh cong.
exit /b 0

:test
call :build_test_impl "%COMMON%" || exit /b 1
"%PROJECT_DIR%%TEST_OUT%"
if errorlevel 1 exit /b 1
exit /b 0

:run_tui
if not exist "%TUI_OUT%" (
    call :build_tui_impl "%COMMON%"
    if errorlevel 1 exit /b 1
)
"%PROJECT_DIR%%TUI_OUT%"
exit /b %errorlevel%

:run_gui
if not exist "%GUI_OUT%" (
    call :build_gui_impl "%COMMON%"
    if errorlevel 1 exit /b 1
)
start "" "%PROJECT_DIR%%GUI_OUT%"
exit /b 0

:clean
call :clean_impl
exit /b 0

:build_tui_impl
echo [BUILD] TUI
"%CXX%" %~1 %INCLUDES% app\main_tui.cpp -o "%TUI_OUT%"
if errorlevel 1 (
    echo [LOI] Build TUI that bai.
    exit /b 1
)
echo [OK] %TUI_OUT%
exit /b 0

:build_gui_impl
echo [BUILD] GUI Win32
"%CXX%" %~1 -DUNICODE -D_UNICODE %INCLUDES% app\main.cpp -o "%GUI_OUT%" -lgdi32 -lcomctl32 -lcomdlg32 -lshell32 -mwindows
if errorlevel 1 (
    echo [LOI] Build GUI that bai.
    exit /b 1
)
echo [OK] %GUI_OUT%
exit /b 0

:build_test_impl
echo [BUILD] Backend tests
"%CXX%" %~1 %INCLUDES% tests\backend_tests.cpp -o "%TEST_OUT%"
if errorlevel 1 (
    echo [LOI] Build test that bai.
    exit /b 1
)
echo [OK] %TEST_OUT%
exit /b 0

:clean_impl
if exist "%TUI_OUT%" del /Q "%TUI_OUT%"
if exist "%GUI_OUT%" del /Q "%GUI_OUT%"
if exist "%TEST_OUT%" del /Q "%TEST_OUT%"
echo [OK] Da xoa file build.
exit /b 0
