@echo off
setlocal EnableExtensions EnableDelayedExpansion
REM Simula N clientes: envia 2 msgs e /quit. Mantém cada janela aberta no final.

set "BUILD=build\Release"
set "SERVER=%BUILD%\tcp_server.exe"
set "CLIENT=%BUILD%\tcp_client.exe"
set "N=5"  REM quantos clientes automatizados

if not exist "%SERVER%" (
  echo(ERRO: servidor nao encontrado em "%SERVER%". Compile antes: scripts\build_windows.bat
  exit /b 1
)
if not exist "%CLIENT%" (
  echo(ERRO: cliente nao encontrado em "%CLIENT%". Compile antes: scripts\build_windows.bat
  exit /b 1
)

REM 1) Servidor em janela própria (mantém aberto)
start "server" cmd /k "%SERVER%"

REM Pequena espera pro servidor subir
timeout /t 1 >nul

REM 2) Dispara N clientes em janelas separadas
for /l %%i in (1,1,%N%) do (
  set "TMPIN=%TEMP%\tcp_client_%%i.in"
  > "!TMPIN!" (
    echo c%%i: hello
    echo c%%i: ping
    echo /quit
  )

  REM Alimenta o cliente pelo STDIN com redirecionamento (<) e mantém a janela aberta
  start "client%%i" cmd /k ""%CLIENT%" < "!TMPIN!" ^& echo( ^& echo([client %%i] terminado. Pressione ENTER para fechar...) ^& pause >nul"
)

echo(Iniciei %N% clientes. Feche a janela do servidor quando terminar.
endlocal
