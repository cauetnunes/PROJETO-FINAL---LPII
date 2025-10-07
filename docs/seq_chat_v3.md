# Diagrama de Sequência — Cliente/Servidor (v3)

```mermaid
sequenceDiagram
    autonumber
    participant C1 as Cliente1
    participant C2 as Cliente2
    participant S as Servidor

    S->>S: listen 127.0.0.1:5000

    C1->>S: connect
    S-->>C1: @id 1
    S-->>C1: history (ate 50)

    C1->>S: [1] ola
    S->>S: adiciona ao historico
    S-->>C2: broadcast [1] ola

    C2->>S: connect
    S-->>C2: @id 2
    S-->>C2: history (ate 50)

    C2->>S: [2] oi
    S->>S: adiciona ao historico
    S-->>C1: broadcast [2] oi

    C1->>S: /quit
    Note over C1,S: cliente fecha escrita e continua lendo ate EOF

    C2->>S: /shutdown
    S->>S: set running false
    S->>S: close listen
    S-->>C1: EOF
    S-->>C2: EOF
    S->>S: fechar sockets e sair
