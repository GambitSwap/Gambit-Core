#include "gambit/p2p_peer.hpp"
#include <unistd.h>
#include <sys/socket.h>
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
    ::send(socketFd_, encoded.data(), encoded.size(), 0);
}

void Peer::recvLoop() {
    while (running_) {
        std::uint8_t header[5];
        ssize_t n = recv(socketFd_, header, 5, MSG_WAITALL);
        if (n <= 0) break;

        std::uint32_t len =
            (header[1] << 24) |
            (header[2] << 16) |
            (header[3] << 8) |
            (header[4]);

        std::vector<std::uint8_t> payload(len);
        n = recv(socketFd_, payload.data(), len, MSG_WAITALL);
        if (n <= 0) break;

        Message msg;
        msg.type = static_cast<MessageType>(header[0]);
        msg.payload = std::move(payload);

        handler_(msg);
    }

    running_ = false;
}

} // namespace gambit
