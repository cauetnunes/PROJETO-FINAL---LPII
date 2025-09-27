#include "tslog/Logger.hpp"
#include <thread>
#include <vector>
#include <string>
#include <iostream>


int main(int argc, char** argv) {
using namespace tslog;


std::string path = (argc > 1 ? argv[1] : "logs.txt");
Logger::instance().set_log_file(path);
Logger::instance().set_min_level(Level::Trace);


const int N_THREADS = 8;
const int MSG_PER_THREAD = 500;


std::vector<std::thread> workers;
workers.reserve(N_THREADS);


for (int i = 0; i < N_THREADS; ++i) {
workers.emplace_back([i]{
for (int j = 0; j < MSG_PER_THREAD; ++j) {
Logger::instance().info("worker=" + std::to_string(i) + " msg=" + std::to_string(j));
}
});
}


for (auto& th : workers) th.join();


Logger::instance().warn("finalizando logger");
Logger::instance().shutdown();


std::cout << "Logs gravados em: " << path << "\n";
return 0;
}