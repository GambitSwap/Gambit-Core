#include "gambit/p2p_peer.hpp"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define SHUT_RDWR SD_BOTH
    #define MSG_WAITALL 0x8
    inline int close(int fd) { return closesocket(fd); }
    typedef int ssize_t;
#else
    #include <unistd.h>
    #include <sys/socket.h>
#endif

#include <stdexcept>

namespace gambit {

Peer::Peer(int socketFd, const std::string& remoteAddr)
    : socketFd_(socketFd), remoteAddr_(remoteAddr) {}

Peer::~Peer() {
    stop();
}

void Peer::start(MessageHandler handler) {
    handler_ = handler;
    running_ = true;
    recvThread_ = std::thread(&Peer::recvLoop, this);
}

void Peer::stop() {
    if (running_) {
        running_ = false;
        shutdown(socketFd_, SHUT_RDWR);
        close(socketFd_);
    }
    if (recvThread_.joinable()) {
        recvThread_.join();
    }
}

void Peer::send(const Message& msg) {
    std::lock_guard<std::mutex> lock(sendMutex_);
    auto encoded = msg.encode();
#ifdef _WIN32
    ::send(socketFd_, reinterpret_cast<const char*>(encoded.data()), static_cast<int>(encoded.size()), 0);
#else
    ::send(socketFd_, encoded.data(), encoded.size(), 0);
#endif
}

void Peer::recvLoop() {
    while (running_) {
        std::uint8_t header[5];
#ifdef _WIN32
        ssize_t n = recv(socketFd_, reinterpret_cast<char*>(header), 5, MSG_WAITALL);
#else
        ssize_t n = recv(socketFd_, header, 5, MSG_WAITALL);
#endif
        if (n <= 0) break;

        std::uint32_t len =
            (header[1] << 24) |
            (header[2] << 16) |
            (header[3] << 8) |
            (header[4]);

        std::vector<std::uint8_t> payload(len);
#ifdef _WIN32
        n = recv(socketFd_, reinterpret_cast<char*>(payload.data()), static_cast<int>(len), MSG_WAITALL);
#else
        n = recv(socketFd_, payload.data(), len, MSG_WAITALL);
#endif
        if (n <= 0) break;

        Message msg;
        msg.type = static_cast<MessageType>(header[0]);
        msg.payload = std::move(payload);

        handler_(msg);
    }

    running_ = false;
}

} // namespace gambit
