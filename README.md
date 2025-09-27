# Projeto 1 — Etapa 1 (v1-logging)
Chat tcp

## 📁 Estrutura do projeto
```
.
├─ CMakeLists.txt
├─ README.md
├─ app/
│  └─ log_demo.cpp              # CLI de estresse (N threads registrando logs)
├─ docs/
│  └─ seq_logger.md             # Diagrama de sequência do logger
├─ include/
│  └─ tslog/
│     ├─ Logger.hpp             # API pública da libtslog
│     ├─ LogLevel.hpp           # Níveis de log e conversão p/ string
│     └─ ThreadSafeQueue.hpp    # Fila thread-safe (cond_var + mutex)
├─ scripts/
│  ├─ build_linux.sh            # Build no Ubuntu
│  └─ build_windows.bat         # Build no Windows (MSVC)
└─ src/
   ├─ Logger.cpp                # Thread worker, formatação e escrita
   └─ platform.hpp              # Utilitário de portabilidade (Windows/POSIX)
```

> Observação: a pasta `build/` e artefatos de Visual Studio/CMake são **gerados** e devem ficar fora do Git (veja `.gitignore`).

---

## ▶️ Como compilar e rodar

### Windows (MSVC) — recomendado
1. Abra **x64 Native Tools Command Prompt for VS 2022**.
2. Na raiz do projeto, **compile**:
   ```bat
   scripts/build_windows.bat
   ```
3. Execute:
   ```bat
   build\Release\log_demo.exe logs.txt
   ```
   > Se você construiu em `Debug`, rode `build\Debug\log_demo.exe logs.txt`.

**Saída esperada**
- Linhas intercaladas de várias threads, sem interleaving por linha.
- Arquivo `logs.txt` criado no diretório atual.
- Com os defaults do `log_demo`, espere **4001** linhas (8 threads × 500 msgs + 1 aviso final).

### Ubuntu (g++)
```bash
chmod +x scripts/build_linux.sh
./scripts/build_linux.sh
./build/log_demo logs.txt
```

---

## 🧪 Testes rápidos
- **Contar linhas (Windows/CMD):**
  ```bat
  find /v /c "" logs.txt
  ```
- **Últimas linhas (PowerShell):**
  ```bat
  powershell -NoProfile -Command "Get-Content -LiteralPath '%CD%\logs.txt' -Tail 10"
  ```
- **Sem linhas vazias:**
  ```bat
  findstr /r "^\s*$" logs.txt && echo "tem linha vazia" || echo "ok, sem linha vazia"
  ```

- **Stress test:** edite `app/log_demo.cpp` e ajuste:
  ```cpp
  const int N_THREADS = 32;
  const int MSG_PER_THREAD = 10000;
  ```
  Recompile e rode:
  ```bat
  script/build_windows.bat
  build\Release\log_demo.exe stress.txt
  ```




