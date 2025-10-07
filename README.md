# Projeto 1 — Chat TCP (Etapas 1–3)

Implementação do chat TCP multiusuário em C++ com **logging concorrente** (libtslog), **servidor** com 1 thread por cliente e **cliente CLI** com prompt por usuário. Mensagens saem no formato **`[ID]: texto`**, o cliente mostra **`[ID]> `** como prompt e o servidor envia um **histórico das últimas 50 mensagens** quando alguém entra.

---

## 📁 Estrutura do projeto

```
.
├─ CMakeLists.txt
├─ README.md
├─ app/
│  ├─ log_demo.cpp         # Etapa 1: stress de logging (N threads)
│  ├─ tcp_server.cpp       # Etapas 2/3: servidor TCP (thread/cliente, broadcast, histórico)
│  └─ tcp_client.cpp       # Etapas 2/3: cliente CLI (stdin/out, /quit)
├─ docs/
│  ├─ seq_logger.md        # Diagrama do logger (v1)
│  └─ seq_chat_v3.md       # Diagrama cliente-servidor (v3)  ← (opcional, se criou)
├─ include/
│  └─ tslog/
│     ├─ Logger.hpp
│     ├─ LogLevel.hpp
│     └─ ThreadSafeQueue.hpp
├─ scripts/
│  ├─ build_linux.sh       # Build no Ubuntu
│  ├─ build_windows.bat    # Build no Windows (MSVC) – autodetecta cmake
│  └─ test_multi_win.bat   # Simula vários clientes no Windows (opcional)
└─ src/
   ├─ Logger.cpp           # Worker do logger + formatação
   ├─ net.hpp              # Camada de compatibilidade sockets (Win/Linux)
   └─ platform.hpp         # Utilitários de portabilidade (se usado)
```

> Obs.: a pasta `build/` e artefatos do Visual Studio/CMake são gerados. **Ficam fora do Git** (veja `.gitignore`).

---

## ▶️ Como compilar e rodar

### Windows (MSVC) — recomendado
1. Abra **x64 Native Tools Command Prompt for VS 2022** (ou VS 2019/Insiders equivalente).
2. Na raiz do projeto, **compile**:
   ```bat
   scripts\build_windows.bat
   ```
3. Rode o **servidor** e **clientes** (cada um em um terminal):
   ```bat
   build\Release\tcp_server.exe
   build\Release\tcp_client.exe
   build\Release\tcp_client.exe
   ```
   - Prompt do cliente: `[#]>`
   - Mensagens: `[ID]: texto`
   - Comandos:
     - `/quit` → o cliente **para de enviar** e **continua recebendo** até o servidor encerrar.
     - `/shutdown` → derruba o servidor de forma limpa.

**Histórico**
- Ao conectar, o cliente recebe **as últimas 50 mensagens** que já rolaram.
- Valor pode ser trocado no código (`MAX_HIST` em `tcp_server.cpp`).

### Ubuntu (g++)
```bash
chmod +x scripts/build_linux.sh
./scripts/build_linux.sh

# Em três terminais:
./build/tcp_server
./build/tcp_client
./build/tcp_client
```

---


## ✅ O que foi implementado
- Servidor TCP **concorrente** (1 thread por cliente).
- **Broadcast** de mensagens para todos os clientes conectados.
- **Logging thread-safe** (libtslog) no server e nos clients.
- Cliente CLI com **prompt** e **redesenho** (não “come” sua linha se alguém falar).
- **Histórico** com mutex (últimas 50 mensagens) enviado ao entrar.
- **/quit** com `shutdown(SEND)` (cliente drena mensagens antes de sair).
- **/shutdown** no servidor (encerra accept, fecha sockets e termina limpo).
