#include "tslog/Logger.hpp"
#include "net.hpp"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>

using namespace tslog;

static const int PORT = 5000;

// Estado global
static std::vector<socket_t> g_clients;
static std::unordered_map<socket_t,int> g_ids; 
static std::mutex g_clients_m;

static std::vector<std::string> g_history;  
static std::mutex g_hist_m;
static constexpr size_t MAX_HIST = 50;

static std::atomic<bool> g_running{true};
static std::atomic<int>  g_next_id{1};

#ifdef _WIN32
static socket_t g_listen = INVALID_SOCKET;
#else
static socket_t g_listen = -1;
#endif

static void add_to_history(std::string line) {
    if (line.empty() || line.back()!='\n') line.push_back('\n');
    std::lock_guard<std::mutex> lk(g_hist_m);
    g_history.push_back(std::move(line));
    if (g_history.size() > MAX_HIST) g_history.erase(g_history.begin());
}

static void send_history(socket_t csock) {
    std::vector<std::string> copy;
    {
        std::lock_guard<std::mutex> lk(g_hist_m);
        copy = g_history;
    }
    for (auto& h : copy) {
        (void)send(csock, h.c_str(), (int)h.size(), 0);
    }
    Logger::instance().info("historico enviado ao novo cliente: " + std::to_string(copy.size()) + " linhas");
}

static int id_of(socket_t s) {
    std::lock_guard<std::mutex> lk(g_clients_m);
    auto it = g_ids.find(s);
    return (it==g_ids.end()) ? -1 : it->second;
}

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

        size_t pos;
        while ((pos = acc.find('\n')) != std::string::npos) {
            std::string payload = acc.substr(0, pos); 
            acc.erase(0, pos + 1);

            // comandos
            if (payload == "/shutdown") {
                Logger::instance().warn("comando /shutdown recebido; encerrando servidor");
                if (g_running.exchange(false)) {
                    if (net_valid(g_listen)) net_close(g_listen);
                }
                continue; // não enviar comando aos demais
            }
            if (payload == "/quit") {
                // Cliente sinaliza saída; não broadcast
                break;
            }

            const int cid = id_of(csock);
            std::string formatted = "[" + std::to_string(cid) + "]: " + payload + "\n";

            add_to_history(formatted);
            broadcast_line(formatted, csock);
        }
        if (!g_running.load()) break;
    }

    // remove e fecha
    {
        std::lock_guard<std::mutex> lk(g_clients_m);
        g_clients.erase(std::remove(g_clients.begin(), g_clients.end(), csock), g_clients.end());
        g_ids.erase(csock);
    }
    net_close(csock);
    Logger::instance().info("cliente saiu");
}

int main() {
    Logger::instance().info("iniciando servidor (fase 3 v3-final)");
    if (net_startup() != 0) {
        Logger::instance().error("Startup falhou");
        return 1;
    }

    g_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (!net_valid(g_listen)) {
        Logger::instance().error("socket() falhou");
        net_cleanup();
        return 2;
    }

    int yes = 1;
#ifdef _WIN32
    setsockopt(g_listen, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
#else
    setsockopt(g_listen, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1

    if (bind(g_listen, (sockaddr*)&addr, sizeof(addr)) != 0) {
        Logger::instance().error("bind() falhou");
        net_close(g_listen);
        net_cleanup();
        return 3;
    }
    if (listen(g_listen, 32) != 0) {
        Logger::instance().error("listen() falhou");
        net_close(g_listen);
        net_cleanup();
        return 4;
    }

    Logger::instance().info("servidor ouvindo em 127.0.0.1:" + std::to_string(PORT));

    while (g_running.load()) {
        sockaddr_in caddr{};
#ifdef _WIN32
        int clen = (int)sizeof(caddr);
#else
        socklen_t clen = sizeof(caddr);
#endif
        socket_t csock = accept(g_listen, (sockaddr*)&caddr, &clen);
        if (!net_valid(csock)) {
            if (!g_running.load()) break;
            Logger::instance().warn("accept() falhou");
            continue;
        }

        // atribui ID e registra
        const int myid = g_next_id.fetch_add(1);
        {
            std::lock_guard<std::mutex> lk(g_clients_m);
            g_clients.push_back(csock);
            g_ids[csock] = myid;
        }

        // informa o ID ao cliente
        std::string you = "@id:" + std::to_string(myid) + "\n";
        (void)send(csock, you.c_str(), (int)you.size(), 0);

        // envia histórico 
        send_history(csock);

        // // avisa aos demais que entrou
        // std::string joined = "[*]: cliente " + std::to_string(myid) + " entrou\n";
        // add_to_history(joined);
        // broadcast_line(joined, csock);

        std::thread(handle_client, csock).detach();
    }

    // encerra remanescentes
    {
        std::lock_guard<std::mutex> lk(g_clients_m);
        for (auto s : g_clients) net_close(s);
        g_clients.clear();
        g_ids.clear();
    }

    if (net_valid(g_listen)) net_close(g_listen);
    net_cleanup();
    Logger::instance().shutdown();
    return 0;
}
