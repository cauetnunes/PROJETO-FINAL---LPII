@echo on
setlocal

REM Este .BAT funciona no "x64 Native Tools Command Prompt for VS 2022"
REM Ele sempre localiza o RAIZ do projeto com base na pasta do script.

REM Caminhos
set "SCRIPT_DIR=%~dp0"
REM raiz do projeto = pasta acima de scripts\
for %%I in ("%SCRIPT_DIR%\..") do set "ROOT=%%~fI"

echo ROOT = "%ROOT%"

REM 1) Gera build na pasta raiz\build
if not exist "%ROOT%\build" mkdir "%ROOT%\build"

REM 2) Gerar projeto (Visual Studio 2022, x64)
cmake -G "Visual Studio 17 2022" -A x64 -S "%ROOT%" -B "%ROOT%\build" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 goto :err

REM 3) Compilar em Release (MSBuild por trás)
cmake --build "%ROOT%\build" --config Release -- /m
if errorlevel 1 goto :err

echo.
echo ===== SUCESSO =====
echo Binario em: "%ROOT%\build\Release\log_demo.exe"
exit /b 0

:err
echo.
echo ===== FALHOU (errorlevel %errorlevel%) =====
exit /b 1
