sequenceDiagram
participant App
participant Logger
participant Queue
participant Worker
participant File


App->>Logger: log(Level,msg)
Logger->>Queue: push(LogRecord)
Note right of Queue: notify_one()
Worker->>Queue: pop_wait(done)
Queue-->>Worker: LogRecord
Worker->>File: write(line) + flush on shutdown