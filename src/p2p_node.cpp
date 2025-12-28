#include "gambit/p2p_node.hpp"
#include "gambit/hash.hpp"
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

    // Minimal stub: seeder bootstrap is application-specific and optional.
    void P2PNode::bootstrapWithSeeder() {
        std::cout << "bootstrapWithSeeder: not implemented; skipping seeder registration\n";
    }

P2PNode::P2PNode(Blockchain& chain, std::uint16_t listenPort)
    : chain_(chain), listenPort_(listenPort), keyPair_(KeyPair::random()) {}

void P2PNode::start() {
    // Setup the DNSSeedManager
    DNSSeedManager dns;

    const uint16_t defaultPort = 30303;

    // ToDo: Limit the ips to no more than 20 peers
    auto seedIPs = dns.resolveAll();
    // ToDo: Check the peer capacity; and join lowest cap first.
    for (const auto& ip : seedIPs) {
        std::cout << "[DNS] Found peer: " << ip << "\n";
        connectTo(ip, defaultPort);
    }

    running_ = true;

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    listenSock_ = static_cast<int>(socket(AF_INET, SOCK_STREAM, 0));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(listenPort_);

    bind(listenSock_, (sockaddr*)&addr, sizeof(addr));
    listen(listenSock_, 16);

    acceptThread_ = std::thread(&P2PNode::acceptLoop, this);
}


// Add the missing methods:
Address P2PNode::myNodeId() const {
    return keyPair_.address();
}

Bytes P2PNode::buildSeederProof(PeerInfo& peer) {
    return ZkSeederClient::buildSeederProof(keyPair_, peer, chain_.chainId());
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

#ifdef _WIN32
    WSACleanup();
#endif
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
    try {
        Transaction tx = Transaction::fromHex(hex);
        std::string err;
        if (chain_.validateTransaction(tx, err)) {
            chain_.addTransaction(tx);
        }
    } catch (...) {
        // Invalid transaction, ignore
    }
}

void P2PNode::handleNewBlock(const Message& msg) {
    std::string hex(msg.payload.begin(), msg.payload.end());
    try {
        Block block = Block::fromHex(hex);
        chain_.addBlock(block);
    } catch (...) {
        // Invalid block, ignore
    }
}

} // namespace gambit
