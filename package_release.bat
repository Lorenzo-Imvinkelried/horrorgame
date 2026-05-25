@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo   GamePS1Horror Release Packager
echo ===================================================
echo.

:: Define target folder
set DIST_DIR=dist\GamePS1Horror_Release

:: 1. Build the game in Release mode
echo [1/4] Configuring CMake in Release mode...
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed!
    goto error
)

echo.
echo [2/4] Building the game...
cmake --build build --config Release
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed!
    goto error
)

:: 2. Prepare dist directory
echo.
echo [3/4] Preparing distribution folder: %DIST_DIR%
if exist dist (
    echo Cleaning existing dist folder...
    rmdir /S /Q dist
)
mkdir %DIST_DIR%

:: 3. Copy binaries and assets from build/bin
echo.
echo [4/4] Copying game files, DLLs, and assets...

copy "build\bin\GamePS1Horror.exe" "%DIST_DIR%\"
copy "build\bin\config.json" "%DIST_DIR%\"
copy "build\bin\libglad.dll" "%DIST_DIR%\"
copy "build\bin\libtinyobjloader.dll" "%DIST_DIR%\"
copy "build\bin\sfml-system-2.dll" "%DIST_DIR%\"
copy "build\bin\sfml-window-2.dll" "%DIST_DIR%\"

:: Copy MSYS2 compiler runtime DLLs so it runs on other PCs
echo.
echo [INFO] Looking for GCC runtime DLLs in MSYS2 UCRT64...
if exist "C:\msys64\ucrt64\bin\libstdc++-6.dll" (
    copy /Y "C:\msys64\ucrt64\bin\libstdc++-6.dll" "%DIST_DIR%\"
    echo Copied libstdc++-6.dll
) else (
    echo [WARNING] libstdc++-6.dll not found in C:\msys64\ucrt64\bin\
)

if exist "C:\msys64\ucrt64\bin\libgcc_s_seh-1.dll" (
    copy /Y "C:\msys64\ucrt64\bin\libgcc_s_seh-1.dll" "%DIST_DIR%\"
    echo Copied libgcc_s_seh-1.dll
) else (
    echo [WARNING] libgcc_s_seh-1.dll not found in C:\msys64\ucrt64\bin\
)

if exist "C:\msys64\ucrt64\bin\libwinpthread-1.dll" (
    copy /Y "C:\msys64\ucrt64\bin\libwinpthread-1.dll" "%DIST_DIR%\"
    echo Copied libwinpthread-1.dll
) else (
    echo [WARNING] libwinpthread-1.dll not found in C:\msys64\ucrt64\bin\
)

:: Copy Assets folder
echo.
echo Copying game assets (shaders, models, etc.)...
xcopy /E /I /Y build\bin\assets %DIST_DIR%\assets

echo.
echo ===================================================
echo   Success! Game package generated successfully.
echo ===================================================
echo.
echo Package path: %cd%\%DIST_DIR%
echo.
echo You can zip the "GamePS1Horror_Release" folder inside "dist\" 
echo and send it to your friend to play!
echo ===================================================
goto end

:error
echo.
echo ===================================================
echo   [FAILED] Packaging failed. See errors above.
echo ===================================================
exit /b 1

:end
exit /b 0
