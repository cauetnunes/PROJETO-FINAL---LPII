#include "tslog/Logger.hpp"
#include "net.hpp"

#include <algorithm>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace tslog;

static const int PORT = 5000;

static std::vector<socket_t> g_clients;
static std::mutex g_clients_m;

static void broadcast_line(const std::string& line, socket_t from) {
    std::lock_guard<std::mutex> lk(g_clients_m);
    for (auto s : g_clients) {
        if (s == from) continue;
        (void)send(s, line.c_str(), (int)line.size(), 0);
    }
}

static void handle_client(socket_t csock) {
    Logger::instance().info("cliente conectado");
    char buf[1024];
    std::string acc;

    for (;;) {
        int n = recv(csock, buf, sizeof(buf), 0);
        if (n <= 0) break;

        acc.append(buf, buf + n);

        // protocolo: cada mensagem termina com '\n'
        size_t pos;
        while ((pos = acc.find('\n')) != std::string::npos) {
            std::string line = acc.substr(0, pos + 1);
            acc.erase(0, pos + 1);
            broadcast_line(line, csock);
        }
    }

    {
        std::lock_guard<std::mutex> lk(g_clients_m);
        g_clients.erase(std::remove(g_clients.begin(), g_clients.end(), csock), g_clients.end());
    }
    net_close(csock);
    Logger::instance().info("cliente saiu");
}

int main() {
    Logger::instance().info("iniciando servidor (fase 2 v2-cli)");
    if (net_startup() != 0) {
        Logger::instance().error("WSAStartup/Startup falhou");
        return 1;
    }

    socket_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (!net_valid(s)) {
        Logger::instance().error("socket() falhou");
        net_cleanup();
        return 2;
    }

    // Reuse address
    int yes = 1;
#ifdef _WIN32
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
#else
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1

    if (bind(s, (sockaddr*)&addr, sizeof(addr)) != 0) {
        Logger::instance().error("bind() falhou");
        net_close(s);
        net_cleanup();
        return 3;
    }
    if (listen(s, 32) != 0) {
        Logger::instance().error("listen() falhou");
        net_close(s);
        net_cleanup();
        return 4;
    }

    Logger::instance().info("servidor ouvindo em 127.0.0.1:" + std::to_string(PORT));

    // Loop principal de accept
    for (;;) {
        sockaddr_in caddr{};
#ifdef _WIN32
        int clen = (int)sizeof(caddr);
#else
        socklen_t clen = sizeof(caddr);
#endif
        socket_t csock = accept(s, (sockaddr*)&caddr, &clen);
        if (!net_valid(csock)) {
            Logger::instance().warn("accept() falhou");
            continue;
        }
        {
            std::lock_guard<std::mutex> lk(g_clients_m);
            g_clients.push_back(csock);
        }
        std::thread(handle_client, csock).detach();
    }

    // (normalmente não chega aqui)
    net_close(s);
    net_cleanup();
    Logger::instance().shutdown();
    return 0;
}
