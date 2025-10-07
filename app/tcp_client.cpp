#include "tslog/Logger.hpp"
#include "net.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

using namespace tslog;

static const int PORT = 5000;

static void print_prompt(int myid) {
    if (myid > 0) std::cout << "[" << myid << "]> ";
    else          std::cout << "[?]> ";
    std::cout.flush();
}

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
    if (inet_pton4("127.0.0.1", &addr.sin_addr) != 1) {
        Logger::instance().error("inet_pton falhou");
        net_close(s);
        net_cleanup();
        return 3;
    }

    if (connect(s, (sockaddr*)&addr, sizeof(addr)) != 0) {
        Logger::instance().error("connect() falhou");
        net_close(s);
        net_cleanup();
        return 4;
    }

    Logger::instance().info("conectado ao servidor 127.0.0.1:5000");

    std::atomic<bool> running{true};
    std::atomic<int> myid{0};

    // Thread que lê do socket e imprime
    std::thread reader([&]{
        std::string acc;
        char buf[1024];
        while (running.load()) {
            int n = recv(s, buf, sizeof(buf), 0);
            if (n <= 0) break;
            acc.append(buf, buf+n);

            size_t pos;
            while ((pos = acc.find('\n')) != std::string::npos) {
                std::string line = acc.substr(0, pos);
                acc.erase(0, pos+1);

                // protocolo de controle do servidor: @id:N
                if (line.rfind("@id:", 0) == 0) {
                    try {
                        myid.store(std::stoi(line.substr(4)));
                    } catch (...) {}
                    // mostra prompt atualizado
                    std::cout << "\r"; // volta ao início da linha
                    print_prompt(myid.load());
                    continue;
                }

                // imprimir a linha recebida e redesenhar o prompt
                std::cout << "\r" << line << "\n";
                print_prompt(myid.load());
            }
        }
        running.store(false);
    });

    // Entrada do usuário
    print_prompt(myid.load());
    std::string line;
    while (running.load() && std::getline(std::cin, line)) {
        if (line == "/quit") {
            line.push_back('\n');
            (void)send(s, line.c_str(), (int)line.size(), 0);
#ifdef _WIN32
            shutdown(s, SD_SEND);
#else
            shutdown(s, SHUT_WR);
#endif
            break;
        } else {
            line.push_back('\n');
            (void)send(s, line.c_str(), (int)line.size(), 0);
        }
        // redesenha prompt depois de enviar
        print_prompt(myid.load());
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    if (reader.joinable()) reader.join();

    net_close(s);
    Logger::instance().shutdown();
    net_cleanup();
    return 0;
}
