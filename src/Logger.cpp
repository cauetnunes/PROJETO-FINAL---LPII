// src/Logger.cpp
#include "tslog/Logger.hpp"
#include "platform.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <mutex>

namespace tslog {

namespace {
std::mutex s_console_mutex; // evita interleaving no console
}

Logger& Logger::instance() {
    static Logger g;
    return g;
}

Logger::Logger() {
    // thread de consumo
    worker_ = std::thread(&Logger::worker_loop, this);
}

Logger::~Logger() {
    shutdown();
}

void Logger::set_log_file(const std::string& path) {
    // modo binário evita transformação de CRLF/LF no Windows
    file_.open(path, std::ios::out | std::ios::app | std::ios::binary);
}

void Logger::log(Level lvl, const std::string& msg) {
    if (lvl < min_level_) return;
    LogRecord rec{lvl, now_iso_utc(), curr_thread_id(), msg};
    queue_.push(std::move(rec));
}

void Logger::shutdown() {
    bool expected = false;
    if (done_.compare_exchange_strong(expected, true)) {
        if (worker_.joinable()) worker_.join();
        if (file_.is_open()) file_.flush();
    }
}

std::string Logger::now_iso_utc() const {
    using namespace std::chrono;
    auto tp = time_point_cast<milliseconds>(system_clock::now());
    auto t  = system_clock::to_time_t(tp);
    auto ms = tp.time_since_epoch() % 1000ms;
    std::tm tm{};
#if TSLOG_PLATFORM_WINDOWS
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setw(3) << std::setfill('0') << ms.count() << "Z";
    return oss.str();
}

std::string Logger::curr_thread_id() const {
    std::ostringstream oss; 
    oss << std::this_thread::get_id(); 
    return oss.str();
}

void Logger::worker_loop() {
    while (true) {
        bool done_flag = done_.load();
        auto rec = queue_.pop_wait(done_flag);
        if (!rec.has_value()) break; // done_ e fila vazia

        std::ostringstream line;
        line << '[' << rec->timestamp << "] [" << to_string(rec->level)
             << "] [tid=" << rec->thread_id << "] " << rec->message << '\n';
        std::string s = line.str();

#ifdef TSLOG_ENABLE_CONSOLE
        {
            std::lock_guard<std::mutex> lk(s_console_mutex);
            std::cout << s;
        }
#endif
        if (file_.is_open()) {
            file_.write(s.data(), static_cast<std::streamsize>(s.size()));
        }
    }
    if (file_.is_open()) file_.flush();
}

} // namespace tslog
