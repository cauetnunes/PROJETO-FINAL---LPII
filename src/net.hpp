#pragma once

#ifdef _WIN32
  #define _WINSOCK_DEPRECATED_NO_WARNINGS
  #include <winsock2.h>
  #include <ws2tcpip.h>
  using socket_t = SOCKET;

  inline int  net_startup() { WSADATA w; return WSAStartup(MAKEWORD(2,2), &w); }
  inline void net_cleanup() { WSACleanup(); }
  inline void net_close(socket_t s) { closesocket(s); }
  inline int  net_last_error() { return WSAGetLastError(); }
  inline bool net_valid(socket_t s) { return s != INVALID_SOCKET; }

  // inet_pton para IPv4 no Windows com char*
  inline int inet_pton4(const char* ip, IN_ADDR* out) {
    return InetPtonA(AF_INET, ip, out);
  }
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <sys/types.h>
  #include <unistd.h>
  #include <cerrno>
  using socket_t = int;

  inline int  net_startup() { return 0; }
  inline void net_cleanup() {}
  inline void net_close(socket_t s) { close(s); }
  inline int  net_last_error() { return errno; }
  inline bool net_valid(socket_t s) { return s >= 0; }

  inline int inet_pton4(const char* ip, in_addr* out) {
    return inet_pton(AF_INET, ip, out);
  }
#endif
