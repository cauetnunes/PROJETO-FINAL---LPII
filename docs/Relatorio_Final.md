# Relatório de Análise com IA — Etapa 3 (v3-final)

**Tema:** Servidor de Chat Multiusuário (TCP)  
**Versão:** `v3-final`

---

## Objetivo
Realizar uma análise crítica assistida por IA do sistema concorrente (cliente/servidor TCP), identificando **possíveis problemas de concorrência** e descrevendo as **mitigações** implementadas no código do projeto. A revisão foi conduzida com apoio de um modelo de linguagem (LLM), usado para checar trechos de código, sugerir correções e validar comportamentos de encerramento.

---

## Visão Geral do Sistema
- **Servidor** TCP com **1 thread por cliente**; cada linha recebida é retransmitida em **broadcast** para os demais.
- **Histórico** (ring buffer) das **últimas 50 mensagens**, enviado a todo novo cliente.
- **Cliente CLI** com prompt `[ID]>`, `/quit` com `shutdown(SEND)` para drenar o que chega antes de sair.
- **Encerramento do servidor** por comando `/shutdown` (fecha o `listen` para destravar `accept()` e encerra limpo).
- **Logging thread-safe** via **libtslog** (biblioteca própria da Etapa 1), que usa fila monitor para serializar escrita e evitar interleaving de linhas.
- Estruturas compartilhadas protegidas por **`std::mutex`**:
  - `g_clients` e `g_ids` (lista/identidade de clientes) — mutex **`g_clients_m`**.
  - `g_history` (ring buffer de mensagens) — mutex **`g_hist_m`**.

---

## Achados da Análise Assistida por IA

### 1) Condições de corrida (race conditions)
**Risco analisado**
- Concorrência em `g_clients`, `g_ids` e `g_history` por múltiplas threads de clientes.
- Acesso simultâneo a recursos de saída (socket/envio) enquanto a lista de clientes é iterada.

**Como está mitigado hoje**
- Uso consistente de **`std::lock_guard<std::mutex>`** em operações de escrita das estruturas compartilhadas.
- **Histórico**: toda inserção/removal sob `g_hist_m`.  
- **Lista/IDs de clientes**: adição/remoção e leitura do ID sob `g_clients_m`.
- **Logger**: a libtslog serializa gravações (uma linha atômica por vez), evitando interleaving de linhas.

**Ponto de atenção (melhoria sugerida)**
- Em `broadcast_line`, o código mantém **`g_clients_m` bloqueado enquanto chama `send()`**. Se um cliente travar no envio, **todas** as threads competindo por `g_clients_m` ficam esperando.
  - **Sugestão**: **copiar** a lista de sockets sob o lock e **enviar fora** do lock.
  ```cpp
  static void broadcast_line(const std::string& line, socket_t from) {
      std::vector<socket_t> snapshot;
      {
          std::lock_guard<std::mutex> lk(g_clients_m);
          snapshot.reserve(g_clients.size());
          for (auto s : g_clients) if (s != from) snapshot.push_back(s);
      }
      for (auto s : snapshot) (void)send(s, line.c_str(), (int)line.size(), 0);
  }
  ```

---

### 2) Deadlocks
**Risco analisado**
- Deadlock por **ordem diferente** de aquisição entre `g_clients_m` e `g_hist_m`.
- Deadlock no encerramento se `accept()`/`recv()` bloquearem indefinidamente.

**Como está mitigado hoje**
- O projeto **nunca aninha** locks; cada mutex é usado em escopos curtos e isolados.
- `/shutdown` faz `net_close(g_listen)` → destrava o `accept()`.
- Cliente `/quit` usa `shutdown(SEND)` → a thread leitora continua até receber EOF, evitando ficar presa.
- Logger tem `shutdown()` explícito, garantindo flush e término da sua thread interna.

**Situação**
- **Baixa probabilidade** de deadlock nos caminhos atuais (bloqueios são curtos e não-ninhados).

---

### 3) Starvation / Sobre‑carga
**Risco analisado**
- Cliente **lento** pode atrasar o broadcast se o envio ficar sob lock (ver item 1).
- Sem fila específica por cliente, um **socket que não consome** pode causar **bloqueio no send()** no nível do SO.

**Como está mitigado hoje**
- Em ambientes locais (loopback) o risco é pequeno; mensagens são curtas e as chamadas `send()` retornam rápido.
- **Logger** não impacta o tráfego de rede (fila é só do logging).

**Sugestões de evolução**
- **Snapshot + envio fora do lock** (citado acima).
- **Timeouts**/modo não-bloqueante com **`select`/`poll`/`epoll`** (Linux) ou **overlapped I/O** (Windows) se for escalar.
- Fila por cliente com **backpressure** (descartar mensagens de clientes muito lentos, conforme política).

---

### 4) Encerramento limpo (shutdown)
**Risco analisado**
- `accept()` preso impede término.
- Threads de clientes paradas em `recv()`/`send()` podem atrasar a saída.

**Como está mitigado hoje**
- Comando `/shutdown` → `g_running=false` e **fecha o socket de listen**; o `accept()` retorna erro e o loop principal termina.
- No final do `main`, o servidor **fecha todos os sockets remanescentes**, garantindo que clientes recebam EOF.
- No cliente, `/quit` envia a linha, chama **`shutdown(SEND)`** e **continua lendo** até EOF (drena as mensagens pendentes).

**Situação**
- Encerramento observado **sem bloqueios** na prática.

---

### 5) Sincronização e monitor (libtslog)
**Risco analisado**
- Perda de sinalização ou interleaving de linhas na escrita de log.

**Como está mitigado hoje**
- A **libtslog** (Etapa 1) encapsula sincronização de produtores/consumidor com primitivas padrão e fila interna; cada **linha de log é atômica** (sem quebra/mescla).  
- `Logger::shutdown()` é chamado explicitamente no término para garantir **flush** e join da thread interna.

---

## Testes realizados (amostras)
- **Histórico**: cliente C conecta após B enviar 2 mensagens → C recebe as 2 imediatamente (ring buffer).  
- **Prompt/redesenho**: mensagens que chegam enquanto você digita **não sobrescrevem** sua linha (o prompt é redesenhado).  
- **/quit**: cliente para de enviar e **continua recebendo** até o servidor encerrar; então sai.  
- **/shutdown**: servidor derruba `accept()`, fecha sockets, encerra limpo; clientes terminam por EOF.

---

## Sugestões Futuras (da IA)
1. **Snapshot em broadcast** (já mostrado) para não manter `g_clients_m` durante `send()`.
2. **Time‑outs** configuráveis para `accept()`/`recv()`/`send()` (robustez em redes reais).
3. **Apelidos** de cliente (nicknames) e comando `/nick` para logs e UX.
4. **Persistência de histórico** (arquivo/rotação) — já que a libtslog facilita logging a disco.
5. **Mecanismo de banimento**/kick e limites de taxa (rate limit) por cliente.
6. **Testes automatizados** com clientes simulados e validação de ordering (golden tests).

---

## Conclusão
A revisão com auxílio de IA confirma que o sistema atende aos requisitos funcionais e apresenta **boas práticas de concorrência** (locks curtos, shutdown limpo, logging thread‑safe).  
O principal ponto de evolução identificado é **evitar `send()` sob o lock** da lista de clientes (usar snapshot). Com esse ajuste, o projeto fica ainda mais robusto para cenários de rede mais adversos e maior número de clientes.

