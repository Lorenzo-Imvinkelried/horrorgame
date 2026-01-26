@echo off
setlocal EnableDelayedExpansion

set "OUTPUT_FILE=full_code.txt"
if exist "%OUTPUT_FILE%" del "%OUTPUT_FILE%"

echo Packing code into %OUTPUT_FILE%...

:: --- CONFIGURATION ---
:: Root files
call :process_file "CMakeLists.txt"

:: Source Code
call :process_folder "src" "*.h"
call :process_folder "src" "*.cpp"

:: Shaders
call :process_folder "assets\shaders" "*.vert"
call :process_folder "assets\shaders" "*.frag"

:: Models
call :process_folder "assets\models" "*.txt"

echo.
echo =========================================
echo Done! All code packed into %OUTPUT_FILE%
echo =========================================
pause
goto :eof

:: ---------------------------------------------------------
:: Functions
:: ---------------------------------------------------------

:process_folder
:: %1 = Folder, %2 = Pattern
if not exist "%~1" goto :eof
pushd "%~1"
for /r %%f in (%~2) do (
    call :write_file "%%~f"
)
popd
goto :eof

:process_file
:: %1 = FilePath
if exist "%~1" (
    call :write_file "%~f1"
)
goto :eof

:write_file
:: %1 = Absolute Path
echo Processing: %~nx1
echo ============================================================================== >> "%~dp0%OUTPUT_FILE%"
echo FILE: %~1 >> "%~dp0%OUTPUT_FILE%"
echo ============================================================================== >> "%~dp0%OUTPUT_FILE%"
echo. >> "%~dp0%OUTPUT_FILE%"
type "%~1" >> "%~dp0%OUTPUT_FILE%"
echo. >> "%~dp0%OUTPUT_FILE%"
echo. >> "%~dp0%OUTPUT_FILE%"
goto :eof
