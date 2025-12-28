#include "gambit/dns_seed.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

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

    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

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

    WSACleanup();
    return results;
}

} // namespace gambit
