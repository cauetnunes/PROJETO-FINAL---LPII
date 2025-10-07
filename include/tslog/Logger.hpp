#pragma once
#include "tslog/LogLevel.hpp"
#include "tslog/ThreadSafeQueue.hpp"
#include <atomic>
#include <fstream>
#include <functional>
#include <string>
#include <thread>
#include <vector>


namespace tslog {


struct LogRecord {
Level level;
std::string timestamp; // ISO-8601
std::string thread_id;
std::string message;
};


class Logger {
public:

static Logger& instance();


// Configuração básica
void set_log_file(const std::string& path);
void set_min_level(Level lvl) { min_level_ = lvl; }


// API de log thread-safe
void log(Level lvl, const std::string& msg);


// Conveniências
void info(const std::string& m) { log(Level::Info, m); }
void debug(const std::string& m) { log(Level::Debug, m); }
void warn(const std::string& m) { log(Level::Warn, m); }
void error(const std::string& m) { log(Level::Error, m); }


// Encerramento limpo (flush)
void shutdown();


private:
Logger();
~Logger();
Logger(const Logger&) = delete;
Logger& operator=(const Logger&) = delete;


void worker_loop();


std::string now_iso_utc() const;
std::string curr_thread_id() const;


private:
ThreadSafeQueue<LogRecord> queue_;
std::atomic<bool> done_{false};
std::thread worker_;


std::ofstream file_;
Level min_level_{Level::Trace};
};


} // namespace tslog