#include "gambit/dns_seed.hpp"
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#define SHUT_RDWR SD_BOTH
inline int close(int fd) { return closesocket(fd); }
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <cstring>

namespace gambit {

    DNSSeedManager::DNSSeedManager() {
        // Add default seeds here
        seeds_.push_back("seed1.gambitswap.com");
        seeds_.push_back("seed2.gambitswap.com");
        seeds_.push_back("seed3.gambitswap.com");
    }
    
    void DNSSeedManager::addSeed(const std::string& domain) {
        seeds_.push_back(domain);
    }
    
    std::vector<std::string> DNSSeedManager::resolveAll() const {
        std::vector<std::string> results;
    
    #ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2,2), &wsa);
    #endif
    
    for (const auto& seed : seeds_) {
        addrinfo hints{}, *res = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(seed.c_str(), nullptr, &hints, &res) != 0) {
            continue;
        }

        for (auto* p = res; p != nullptr; p = p->ai_next) {
            char ip[INET_ADDRSTRLEN];
            sockaddr_in* ipv4 = (sockaddr_in*)p->ai_addr;

            inet_ntop(AF_INET, &(ipv4->sin_addr), ip, INET_ADDRSTRLEN);
            results.push_back(ip);
        }

        freeaddrinfo(res);
    }
    
    #ifdef _WIN32
        WSACleanup();
    #endif
        return results;
    }
    
} // namespace gambit
