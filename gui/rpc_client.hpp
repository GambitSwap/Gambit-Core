#pragma once
#include <string>
#include <cstdint>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include "nlohmann/json.hpp"
#include "block_info.hpp"

using json = nlohmann::json;

namespace gambit {

struct BlockInfo {
    std::string hash;
    std::string parentHash;
    uint64_t number;
    size_t txCount;
};

class RpcClient {
public:
    explicit RpcClient(const std::string& url) : url_(url) {
        parseUrl(url);
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    }
    
    ~RpcClient() {
#ifdef _WIN32
        WSACleanup();
#endif
    }
    
    uint64_t getBlockNumber() {
        std::string response = call("eth_blockNumber", "[]");
        std::string result = extractResult(response);
        return hexToUint64(result);
    }
    
    uint64_t getChainId() {
        std::string response = call("eth_chainId", "[]");
        std::string result = extractResult(response);
        return hexToUint64(result);
    }
    
    uint64_t getBalance(const std::string& address) {
        std::string response = call("eth_getBalance", "[\"" + address + "\", \"latest\"]");
        std::string result = extractResult(response);
        return hexToUint64(result);
    }
    
    BlockInfo getBlockByNumber(uint64_t blockNum) {
        std::string hexNum = "0x" + uint64ToHex(blockNum);
        std::string response = call("eth_getBlockByNumber", "[\"" + hexNum + "\", false]");
        return parseBlockInfo(response);
    }
    
    std::string sendRawTransaction(const std::string& txHex) {
        std::string response = call("eth_sendRawTransaction", "[\"" + txHex + "\"]");
        return extractResult(response);
    }
    
private:
    std::string url_;
    std::string host_;
    uint16_t port_ = 8545;
    std::string path_ = "/";
    
    void parseUrl(const std::string& url) {
        // Simple URL parsing: http://host:port/path
        std::string remaining = url;
        
        // Remove protocol
        if (remaining.rfind("http://", 0) == 0) {
            remaining = remaining.substr(7);
        } else if (remaining.rfind("https://", 0) == 0) {
            remaining = remaining.substr(8);
        }
        
        // Find path
        size_t pathPos = remaining.find('/');
        if (pathPos != std::string::npos) {
            path_ = remaining.substr(pathPos);
            remaining = remaining.substr(0, pathPos);
        }
        
        // Find port
        size_t portPos = remaining.find(':');
        if (portPos != std::string::npos) {
            host_ = remaining.substr(0, portPos);
            port_ = static_cast<uint16_t>(std::stoi(remaining.substr(portPos + 1)));
        } else {
            host_ = remaining;
        }
    }
    
    std::string call(const std::string& method, const std::string& params) {
        static uint64_t requestId = 1;
        
        std::string body = "{\"jsonrpc\":\"2.0\",\"method\":\"" + method + 
                          "\",\"params\":" + params + 
                          ",\"id\":" + std::to_string(requestId++) + "}";
        
        std::string request = "POST " + path_ + " HTTP/1.1\r\n";
        request += "Host: " + host_ + "\r\n";
        request += "Content-Type: application/json\r\n";
        request += "Content-Length: " + std::to_string(body.size()) + "\r\n";
        request += "Connection: close\r\n\r\n";
        request += body;
        
        return sendRequest(request);
    }
    
    std::string sendRequest(const std::string& request) {
        int sock = static_cast<int>(socket(AF_INET, SOCK_STREAM, 0));
        if (sock < 0) {
            throw std::runtime_error("Failed to create socket");
        }
        
        struct hostent* server = gethostbyname(host_.c_str());
        if (!server) {
            closeSocket(sock);
            throw std::runtime_error("Failed to resolve host: " + host_);
        }
        
        struct sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port_);
        std::memcpy(&serverAddr.sin_addr.s_addr, server->h_addr, server->h_length);
        
        if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            closeSocket(sock);
            throw std::runtime_error("Failed to connect to " + host_ + ":" + std::to_string(port_));
        }
        
        // Send request
        if (send(sock, request.c_str(), static_cast<int>(request.size()), 0) < 0) {
            closeSocket(sock);
            throw std::runtime_error("Failed to send request");
        }
        
        // Receive response
        std::string response;
        char buffer[4096];
        int received;
        while ((received = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[received] = '\0';
            response += buffer;
        }
        
        closeSocket(sock);
        
        // Extract body from HTTP response
        size_t bodyStart = response.find("\r\n\r\n");
        if (bodyStart != std::string::npos) {
            return response.substr(bodyStart + 4);
        }
        
        return response;
    }
    
    void closeSocket(int sock) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
    }
    
    uint64_t hexToUint64(const std::string& hex) {
        std::string clean = hex;
        if (clean.rfind("0x", 0) == 0 || clean.rfind("0X", 0) == 0) {
            clean = clean.substr(2);
        }
        if (clean.empty()) return 0;
        return std::stoull(clean, nullptr, 16);
    }
    
    std::string uint64ToHex(uint64_t val) {
        if (val == 0) return "0";
        
        std::string result;
        while (val > 0) {
            int digit = val & 0xF;
            result = "0123456789abcdef"[digit] + result;
            val >>= 4;
        }
        return result;
    }
};

} // namespace gambit

