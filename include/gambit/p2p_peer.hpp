#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <vector>

#include "gambit/p2p_message.hpp"

namespace gambit {

class Peer {
public:
    using MessageHandler = std::function<void(const Message&)>;

    Peer(int socketFd, const std::string& remoteAddr);
    ~Peer();

    void start(MessageHandler handler);
    void send(const Message& msg);
    void stop();

    std::string remoteAddress() const { return remoteAddr_; }

private:
    int socketFd_;
    std::string remoteAddr_;
    std::thread recvThread_;
    std::atomic<bool> running_{false};
    MessageHandler handler_;
    std::mutex sendMutex_;

    void recvLoop();
};

} // namespace gambit
