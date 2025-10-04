#include "tslog/Logger.hpp"
#include "net.hpp"

#include <iostream>
#include <string>
#include <thread>

using namespace tslog;

static const int PORT = 5000;

int main() {
    if (net_startup() != 0) {
        Logger::instance().error("Startup falhou");
        return 1;
    }

    socket_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (!net_valid(s)) {
        Logger::instance().error("socket() falhou");
        net_cleanup();
        return 2;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
#ifdef _WIN32
    if (inet_pton4("127.0.0.1", &addr.sin_addr) != 1) {
        Logger::instance().error("inet_pton falhou");
        return 3;
    }
#else
    if (inet_pton4("127.0.0.1", &addr.sin_addr) != 1) {
        Logger::instance().error("inet_pton falhou");
        return 3;
    }
#endif

    if (connect(s, (sockaddr*)&addr, sizeof(addr)) != 0) {
        Logger::instance().error("connect() falhou");
        net_close(s);
        net_cleanup();
        return 4;
    }

    Logger::instance().info("conectado ao servidor 127.0.0.1:5000");
    std::atomic<bool> running{true};

    // Thread que lê do socket e imprime
    std::thread reader([&]{
        char buf[1024];
        while (running.load()) {
            int n = recv(s, buf, sizeof(buf)-1, 0);
            if (n <= 0) break;
            buf[n] = '\0';
            std::cout << buf; // já vem com '\n'
        }
        running.store(false);
    });

    // Thread principal: lê do teclado e envia
    std::string line;
    while (running.load() && std::getline(std::cin, line)) {
        line.push_back('\n');
        (void)send(s, line.c_str(), (int)line.size(), 0);
        if (line == "/quit\n") {
            running.store(false);
            break;
        }
    }

    net_close(s);
    if (reader.joinable()) reader.join();
    Logger::instance().shutdown();
    net_cleanup();
    return 0;
}
