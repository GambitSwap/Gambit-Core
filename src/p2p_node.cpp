#include "gambit/p2p_node.hpp"
#include "gambit/hash.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

namespace gambit {

P2PNode::P2PNode(Blockchain& chain, std::uint16_t listenPort)
    : chain_(chain), listenPort_(listenPort) {}

void P2PNode::start() {
    running_ = true;

    listenSock_ = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(listenPort_);

    bind(listenSock_, (sockaddr*)&addr, sizeof(addr));
    listen(listenSock_, 16);

    acceptThread_ = std::thread(&P2PNode::acceptLoop, this);
}

void P2PNode::stop() {
    running_ = false;
    shutdown(listenSock_, SHUT_RDWR);
    close(listenSock_);

    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }

    std::lock_guard<std::mutex> lock(peersMutex_);
    for (auto& p : peers_) {
        p->stop();
    }
}

void P2PNode::acceptLoop() {
    while (running_) {
        sockaddr_in client{};
        socklen_t len = sizeof(client);
        int fd = accept(listenSock_, (sockaddr*)&client, &len);
        if (fd < 0) continue;

        std::string addr = inet_ntoa(client.sin_addr);

        auto peer = std::make_shared<Peer>(fd, addr);
        {
            std::lock_guard<std::mutex> lock(peersMutex_);
            peers_.push_back(peer);
        }

        peer->start([this, peer](const Message& msg) {
            onMessage(msg, peer);
        });
    }
}

void P2PNode::connectTo(const std::string& host, std::uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return;
    }

    auto peer = std::make_shared<Peer>(fd, host);
    {
        std::lock_guard<std::mutex> lock(peersMutex_);
        peers_.push_back(peer);
    }

    peer->start([this, peer](const Message& msg) {
        onMessage(msg, peer);
    });
}

void P2PNode::broadcastNewTx(const Transaction& tx) {
    Message msg;
    msg.type = MessageType::NEW_TX;

    std::string hex = tx.toHex();
    msg.payload.assign(hex.begin(), hex.end());

    std::lock_guard<std::mutex> lock(peersMutex_);
    for (auto& p : peers_) {
        p->send(msg);
    }
}

void P2PNode::broadcastNewBlock(const Block& block) {
    Message msg;
    msg.type = MessageType::NEW_BLOCK;

    std::string hex = block.toHex();
    msg.payload.assign(hex.begin(), hex.end());

    std::lock_guard<std::mutex> lock(peersMutex_);
    for (auto& p : peers_) {
        p->send(msg);
    }
}

void P2PNode::onMessage(const Message& msg, std::shared_ptr<Peer> peer) {
    switch (msg.type) {
        case MessageType::NEW_TX:
            handleNewTx(msg);
            break;
        case MessageType::NEW_BLOCK:
            handleNewBlock(msg);
            break;
        default:
            break;
    }
}

void P2PNode::handleNewTx(const Message& msg) {
    std::string hex(msg.payload.begin(), msg.payload.end());
    // TODO: implement Transaction::fromHex
    // For now, ignore
}

void P2PNode::handleNewBlock(const Message& msg) {
    std::string hex(msg.payload.begin(), msg.payload.end());
    // TODO: implement Block::fromHex
    // For now, ignore
}

} // namespace gambit
