#pragma once
#include <thread>
#include <atomic>
#include <cstdint>
#include <string>

#include "gambit/blockchain.hpp"

namespace gambit {

class RpcServer {
public:
    RpcServer(Blockchain& chain, std::uint16_t port);

    void start();
    void stop();

private:
    Blockchain& chain_;
    std::uint16_t port_;
    int listenSock_{-1};
    std::thread acceptThread_;
    std::atomic<bool> running_{false};

    void acceptLoop();
    void handleClient(int clientFd);

    std::string handleRequest(const std::string& httpReq);
    std::string handleJsonRpc(const std::string& json);

    // JSON-RPC method handlers
    std::string handle_blockNumber(const std::string& id);
    std::string handle_getBalance(const std::string& id, const std::string& addrHex);
    std::string handle_sendRawTransaction(const std::string& id, const std::string& txHex);
    
    std::string handle_getBlockByNumber(const std::string& id, const std::string& numHex);
    std::string handle_getBlockByHash(const std::string& id, const std::string& hashHex);
    std::string handle_getTransactionByHash(const std::string& id, const std::string& hashHex);
    std::string handle_getTransactionCount(const std::string& id, const std::string& addrHex);    

    // Tiny helpers
    static std::string httpResponse(const std::string& body, const std::string& status = "200 OK");
    static std::string jsonError(const std::string& id, int code, const std::string& message);
    static std::string jsonResult(const std::string& id, const std::string& resultJson);

    // Extremely naive JSON helpers (string-based, assumes well-formed input)
    static std::string jsonExtractStringParam(const std::string& json, const std::string& key);
    static std::string jsonExtractParamByIndex(const std::string& json, std::size_t index);
    static std::string jsonExtractId(const std::string& json);
    static std::string jsonExtractMethod(const std::string& json);


};

} // namespace gambit
