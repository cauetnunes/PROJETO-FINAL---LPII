cmake_minimum_required(VERSION 3.16)
project(lpii_tslog LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Opção para ativar logs no console
option(TSLOG_ENABLE_CONSOLE "Enable console logging" ON)


# Threads portáveis
find_package(Threads REQUIRED)


add_library(tslog
src/Logger.cpp
)


target_include_directories(tslog PUBLIC include)


target_compile_definitions(tslog PRIVATE $<$<BOOL:${TSLOG_ENABLE_CONSOLE}>:TSLOG_ENABLE_CONSOLE=1>)


target_link_libraries(tslog PRIVATE Threads::Threads)


add_executable(log_demo app/log_demo.cpp)


target_link_libraries(log_demo PRIVATE tslog)

add_executable(tcp_server app/tcp_server.cpp)
target_link_libraries(tcp_server PRIVATE tslog)
target_include_directories(tcp_server PRIVATE src)  # precisa por causa do net.hpp

add_executable(tcp_client app/tcp_client.cpp)
target_link_libraries(tcp_client PRIVATE tslog)
target_include_directories(tcp_client PRIVATE src)  # precisa por causa do net.hpp

# Winsock no Windows
if (WIN32)
  target_link_libraries(tcp_server PRIVATE ws2_32)
  target_link_libraries(tcp_client PRIVATE ws2_32)
endif()


# No Windows, garantir modo UTF-8 (opcional, MSVC)
if (MSVC)
add_compile_options(/utf-8)
endif()