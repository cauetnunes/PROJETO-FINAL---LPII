#pragma once
#include <condition_variable>
#include <mutex>
#include <queue>
#include <optional>


template <typename T>
class ThreadSafeQueue {
public:
void push(T value) {
{
std::lock_guard<std::mutex> lock(m_);
q_.push(std::move(value));
}
cv_.notify_one();
}


// Bloqueia até ter item ou terminar
std::optional<T> pop_wait(bool& done) {
std::unique_lock<std::mutex> lock(m_);
cv_.wait(lock, [&]{ return !q_.empty() || done; });
if (q_.empty()) return std::nullopt;
T v = std::move(q_.front()); q_.pop();
return v;
}


bool empty() const {
std::lock_guard<std::mutex> lock(m_);
return q_.empty();
}


private:
mutable std::mutex m_;
std::condition_variable cv_;
std::queue<T> q_;
};