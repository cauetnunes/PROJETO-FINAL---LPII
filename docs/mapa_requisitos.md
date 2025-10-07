# Mapa de Requisitos → Código (v3)

| Requisito                                             | Onde/Código                                                                 |
|-------------------------------------------------------|------------------------------------------------------------------------------|
| Servidor concorrente (uma thread por cliente)         | `app/tcp_server.cpp` (`std::thread(handle_client).detach()`)                |
| Broadcast para os demais clientes                     | `broadcast_line()` em `app/tcp_server.cpp`                                  |
| Proteção de estruturas (lista/IDs/histórico)          | `g_clients_m`, `g_hist_m` (escopos curtos com `std::lock_guard`)            |
| Histórico das últimas N mensagens                     | `g_history` (+ `MAX_HIST`) e `send_history()` em `app/tcp_server.cpp`       |
| Cliente CLI (prompt, envio e recepção)                | `app/tcp_client.cpp` (threads: teclado→socket e socket→console)             |
| `/quit` com drenagem (shutdown de escrita)            | `tcp_client.cpp` (`shutdown(SEND)` e leitura até EOF)                        |
| `/shutdown` do servidor (término limpo)               | `tcp_server.cpp` (flag, `net_close(g_listen)`, fechamento de sockets)       |
| Logging concorrente (sem interleaving)                | `libtslog` (Etapa 1): `include/tslog/*`, `src/Logger.cpp` + `Logger::shutdown()` |
| Portabilidade Win/Linux (sockets utilitários)         | `src/net.hpp`                                                                |
| Scripts de build/teste                                | `scripts/build_windows.bat`, `scripts/build_linux.sh`, `scripts/test_multi_win.bat` |
