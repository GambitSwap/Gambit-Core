#include "gambit/rpc_server.hpp"
#include "gambit/hash.hpp"
#include "gambit/address.hpp"
#include "gambit/transaction.hpp"
#include "gambit/p2p_node.hpp"
#include "gambit/zk_seeder.hpp"
#include "nlohmann/json.hpp"

#include <map>

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
#include <iostream>

using json = nlohmann::json;

namespace gambit
{

    RpcServer::RpcServer(Blockchain &chain, std::uint16_t port)
        : chain_(chain), port_(port) {}

    void RpcServer::start()
    {
        running_ = true;

#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

        listenSock_ = static_cast<int>(socket(AF_INET, SOCK_STREAM, 0));

#ifdef _WIN32
        char opt = 1;
        setsockopt(listenSock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#else
        int opt = 1;
        setsockopt(listenSock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port_);

        if (bind(listenSock_, (sockaddr *)&addr, sizeof(addr)) < 0)
        {
            std::perror("RpcServer bind");
            return;
        }

        if (listen(listenSock_, 16) < 0)
        {
            std::perror("RpcServer listen");
            return;
        }

        acceptThread_ = std::thread(&RpcServer::acceptLoop, this);
        std::cout << "[RPC] Listening on port " << port_ << "\n";
    }

    void RpcServer::stop()
    {
        running_ = false;
        if (listenSock_ >= 0)
        {
            shutdown(listenSock_, SHUT_RDWR);
            close(listenSock_);
            listenSock_ = -1;
        }
        if (acceptThread_.joinable())
        {
            acceptThread_.join();
        }
#ifdef _WIN32
        WSACleanup();
#endif
    }

    void RpcServer::acceptLoop()
    {
        while (running_)
        {
            sockaddr_in client{};
            socklen_t len = sizeof(client);
            int fd = accept(listenSock_, (sockaddr *)&client, &len);
            if (fd < 0)
            {
                if (!running_)
                    break;
                continue;
            }

            std::thread(&RpcServer::handleClient, this, fd).detach();
        }
    }

    void RpcServer::handleClient(int clientFd)
    {
        constexpr std::size_t BUF_SIZE = 8192;
        char buf[BUF_SIZE];
#ifdef _WIN32
        int n = recv(clientFd, buf, static_cast<int>(BUF_SIZE), 0);
#else
        ssize_t n = recv(clientFd, buf, BUF_SIZE, 0);
#endif
        if (n <= 0)
        {
            close(clientFd);
            return;
        }

        std::string req(buf, buf + n);
        std::string resp = handleRequest(req);

#ifdef _WIN32
        send(clientFd, resp.data(), static_cast<int>(resp.size()), 0);
#else
        send(clientFd, resp.data(), resp.size(), 0);
#endif
        close(clientFd);
    }

    std::string RpcServer::handleRequest(const std::string &httpReq)
    {
        // Very naive HTTP parsing: look for body after double CRLF
        std::cout << "[DEBUG] Received HTTP request: " << httpReq << std::endl;
        auto pos = httpReq.find("\r\n\r\n");
        if (pos == std::string::npos)
        {
            return httpResponse(R"({"jsonrpc":"2.0","error":{"code":-32700,"message":"Parse error"},"id":null})");
        }

        std::string body = httpReq.substr(pos + 4);
        std::cout << "[DEBUG] Extracted JSON body: '" << body << "'" << std::endl;
        std::string jsonResp = handleJsonRpc(body);
        return httpResponse(jsonResp);
    }

    std::string RpcServer::handleJsonRpc(const std::string &jsonStr)
    {
        std::string id = "null";
        std::cout << "[DEBUG] About to parse JSON body (len=" << jsonStr.size() << "): '" << jsonStr << "'" << std::endl;
        try
        {
            json req = json::parse(jsonStr);
            std::cout << "[DEBUG] Parsed JSON: " << req.dump() << std::endl;
            if (req.contains("method"))
            {
                std::cout << "[DEBUG] Method: " << req["method"] << std::endl;
            }
            if (req.contains("params"))
            {
                std::cout << "[DEBUG] Params: " << req["params"].dump() << std::endl;
            }
            if (req.contains("id"))
            {
                id = req["id"].dump();
            }
            std::string method = req["method"];

            if (method == "eth_blockNumber")
            {
                return handle_blockNumber(id);
            }
            else if (method == "eth_getBalance")
            {
                std::string addr = req["params"][0];
                return handle_getBalance(id, addr);
            }
            else if (method == "eth_sendRawTransaction")
            {
                std::string txHex = req["params"][0];
                return handle_sendRawTransaction(id, txHex);
            }
            else if (method == "net_version")
            {
                // chainId as decimal string
                // std::string chainIdStr = std::to_string(chain_.chain().front().index == 0 ? 1337 : 1);
                std::string chainIdStr = std::to_string(chain_.chainId());
                return jsonResult(id, "\"" + chainIdStr + "\"");
            }
            else if (method == "eth_getBlockByNumber")
            {
                std::string num = req["params"][0];
                return handle_getBlockByNumber(id, num);
            }
            else if (method == "eth_getBlockByHash")
            {
                std::string hash = req["params"][0]; // Properly handles quotes, hex, etc.
                return handle_getBlockByHash(id, hash);
            }
            else if (method == "eth_getTransactionByHash")
            {
                std::string h = req["params"][0];
                return handle_getTransactionByHash(id, h);
            }
            else if (method == "eth_getTransactionCount")
            {
                std::string addr = req["params"][0];
                return handle_getTransactionCount(id, addr);
            }

            // TODO: miner_start, miner_stop, miner_setInterval, eth_getWork, eth_submitWork
            // These require passing a Miner reference to RpcServer

            return jsonError(id, -32601, "Method not found");
        }
        catch (const std::exception &e)
        {
            std::cout << "[DEBUG] JSON parse error: " << e.what() << "\n";
            std::cout << "[DEBUG] Raw JSON: " << jsonStr << "\n";
            return jsonError(id, -32799, "Parse error");
        }
    }

    std::string RpcServer::handle_blockNumber(const std::string &id)
    {
        std::uint64_t height = chain_.chain().empty() ? 0 : chain_.chain().back().index;
        // Return as hex, like eth_blockNumber
        char buf[32];
        std::snprintf(buf, sizeof(buf), "0x%llx", static_cast<unsigned long long>(height));
        return jsonResult(id, "\"" + std::string(buf) + "\"");
    }

    std::string RpcServer::handle_getBalance(const std::string &id, const std::string &addrHex)
    {
        try
        {
            Address addr = Address::fromHex(addrHex);
            const Account *acc = chain_.state().get(addr);
            std::uint64_t bal = acc ? acc->balance : 0;

            char buf[64];
            std::snprintf(buf, sizeof(buf), "0x%llx", static_cast<unsigned long long>(bal));
            return jsonResult(id, "\"" + std::string(buf) + "\"");
        }
        catch (...)
        {
            return jsonError(id, -32602, "Invalid address");
        }
    }

    std::string RpcServer::handle_sendRawTransaction(const std::string &id, const std::string &txHex)
    {
        try
        {
            Transaction tx = Transaction::fromHex(txHex);

            std::string err;
            if (!chain_.validateTransaction(tx, err))
            {
                return jsonError(id, -32000, err);
            }

            chain_.addTransaction(tx);

            std::string txHash = tx.hash.empty() ? tx.computeHash() : tx.hash;
            return jsonResult(id, "\"" + txHash + "\"");
        }
        catch (const std::exception &e)
        {
            return jsonError(id, -32602, e.what());
        }
    }

    std::string RpcServer::handle_getBlockByNumber(const std::string &id, const std::string &numHex)
    {
        uint64_t num = std::stoull(numHex, nullptr, 16);

        const auto &chain = chain_.chain();
        if (num >= chain.size())
        {
            return jsonResult(id, "null");
        }

        const Block &b = chain[num];

        std::string out = "{"
                          "\"number\":\"0x" +
                          numHex + "\","
                                   "\"hash\":\"" +
                          b.hash + "\","
                                   "\"parentHash\":\"" +
                          b.prevHash + "\","
                                       "\"stateRoot\":\"" +
                          b.stateAfter + "\","
                                         "\"txRoot\":\"" +
                          b.txRoot + "\","
                                     "\"timestamp\":\"0x" +
                          toHex(rlp::encodeUint(b.timestamp)) + "\""
                                                                "}";

        return jsonResult(id, out);
    }

    std::string RpcServer::handle_getBlockByHash(const std::string &id, const std::string &hashHex)
    {
        std::cout << "[DEBUG] Searching for hash: " << hashHex << "\n";
        
        const auto &chain = chain_.chain();
        std::cout << "[DEBUG] Chain has " << chain.size() << " blocks\n";
        
        for (const auto &b : chain)
        {
            std::cout << "[DEBUG] Block #" << b.index << " hash: " << b.hash << "\n";
            
            // Normalize hashes for comparison (strip 0x prefix)
            std::string storedHash = b.hash;
            std::string searchHash = hashHex;
            
            if (storedHash.rfind("0x", 0) == 0 || storedHash.rfind("0X", 0) == 0)
                storedHash = storedHash.substr(2);
            
            if (searchHash.rfind("0x", 0) == 0 || searchHash.rfind("0X", 0) == 0)
                searchHash = searchHash.substr(2);
            
            if (storedHash == searchHash)
            {
                std::cout << "[DEBUG] FOUND BLOCK #" << b.index << "\n";
                std::string out = "{"
                                  "\"number\":\"0x" + toHex(rlp::encodeUint(b.index)) + "\","
                                  "\"hash\":\"0x" + b.hash + "\","
                                  "\"parentHash\":\"0x" + b.prevHash + "\","
                                  "\"stateRoot\":\"0x" + b.stateAfter + "\","
                                  "\"txRoot\":\"0x" + b.txRoot + "\","
                                  "\"timestamp\":\"0x" + toHex(rlp::encodeUint(b.timestamp)) + "\""
                                  "}";
                return jsonResult(id, out);
            }
        }
        std::cout << "[DEBUG] No matching block found\n";
        return jsonResult(id, "null");
    }

    std::string RpcServer::handle_getTransactionByHash(const std::string &id, const std::string &hashHex)
    {
        // Search mempool
        for (const auto &tx : chain_.mempool())
        {
            if (tx.hash == hashHex)
            {
                std::string out = "{"
                                  "\"hash\":\"" +
                                  tx.hash + "\","
                                            "\"from\":\"" +
                                  tx.from.toHex() + "\","
                                                    "\"to\":\"" +
                                  tx.to.toHex() + "\","
                                                  "\"value\":\"0x" +
                                  toHex(rlp::encodeUint(tx.value)) + "\","
                                                                     "\"nonce\":\"0x" +
                                  toHex(rlp::encodeUint(tx.nonce)) + "\""
                                                                     "}";
                return jsonResult(id, out);
            }
        }

        // Search blocks
        for (const auto &b : chain_.chain())
        {
            for (const auto &tx : b.transactions)
            {
                if (tx.hash == hashHex)
                {
                    std::string out = "{"
                                      "\"hash\":\"" +
                                      tx.hash + "\","
                                                "\"blockHash\":\"" +
                                      b.hash + "\","
                                               "\"blockNumber\":\"0x" +
                                      toHex(rlp::encodeUint(b.index)) + "\","
                                                                        "\"from\":\"" +
                                      tx.from.toHex() + "\","
                                                        "\"to\":\"" +
                                      tx.to.toHex() + "\","
                                                      "\"value\":\"0x" +
                                      toHex(rlp::encodeUint(tx.value)) + "\","
                                                                         "\"nonce\":\"0x" +
                                      toHex(rlp::encodeUint(tx.nonce)) + "\""
                                                                         "}";
                    return jsonResult(id, out);
                }
            }
        }

        return jsonResult(id, "null");
    }

    std::string RpcServer::handle_getTransactionCount(const std::string &id, const std::string &addrHex)
    {
        try
        {
            Address addr = Address::fromHex(addrHex);
            const Account *acc = chain_.state().get(addr);
            uint64_t nonce = acc ? acc->nonce : 0;

            char buf[32];
            std::snprintf(buf, sizeof(buf), "0x%llx", (unsigned long long)nonce);
            return jsonResult(id, "\"" + std::string(buf) + "\"");
        }
        catch (...)
        {
            return jsonError(id, -32602, "Invalid address");
        }
    }

    // ---------- HTTP + JSON helpers ----------

    std::string RpcServer::httpResponse(const std::string &body, const std::string &status)
    {
        std::string resp;
        resp += "HTTP/1.1 " + status + "\r\n";
        resp += "Content-Type: application/json\r\n";
        resp += "Content-Length: " + std::to_string(body.size()) + "\r\n";
        resp += "Connection: close\r\n";
        resp += "\r\n";
        resp += body;
        return resp;
    }

    std::string RpcServer::jsonError(const std::string &id, int code, const std::string &message)
    {
        std::string safeId = id.empty() ? "null" : id;
        return R"({"jsonrpc":"2.0","error":{"code":)" +
               std::to_string(code) +
               R"(,"message":")" + message + R"("},"id":)" +
               safeId + "}";
    }

    std::string RpcServer::jsonResult(const std::string &id, const std::string &resultJson)
    {
        std::string safeId = id.empty() ? "null" : id;
        return R"({"jsonrpc":"2.0","result":)" + resultJson + R"(,"id":)" + safeId + "}";
    }

    // These JSON helpers are intentionally naive and assume well-formed JSON-RPC 2.0 like:
    // {"jsonrpc":"2.0","method":"eth_method","params":["0x..."],"id":1}

    std::string RpcServer::jsonExtractStringParam(const std::string &json, const std::string &key)
    {
        // Try quoted version first: "key":"value"
        auto pos = json.find("\"" + key + "\"");
        if (pos != std::string::npos)
        {
            pos = json.find(':', pos);
            if (pos == std::string::npos)
                return "";
            pos = json.find('"', pos);
            if (pos == std::string::npos)
                return "";
            auto end = json.find('"', pos + 1);
            if (end == std::string::npos)
                return "";
            return json.substr(pos + 1, end - pos - 1);
        }

        // Try unquoted version: key:value (for malformed JSON from curl)
        pos = json.find(key + ":");
        if (pos == std::string::npos)
            return "";
        pos = json.find(':', pos);
        if (pos == std::string::npos)
            return "";
        auto start = json.find_first_not_of(" \t\n\r", pos + 1);
        if (start == std::string::npos)
            return "";
        auto end = json.find_first_of(",}", start);
        if (end == std::string::npos)
            end = json.size();
        // Remove quotes if present
        std::string value = json.substr(start, end - start);
        if (!value.empty() && value.front() == '"' && value.back() == '"')
            value = value.substr(1, value.size() - 2);
        return value;
    }

    std::string RpcServer::jsonExtractId(const std::string &json)
    {
        // Try quoted version first: "id":"value"
        auto pos = json.find("\"id\"");
        if (pos != std::string::npos)
        {
            pos = json.find(':', pos);
            if (pos == std::string::npos)
                return "null";
            auto start = json.find_first_not_of(" \t\n\r", pos + 1);
            if (start == std::string::npos)
                return "null";
            auto end = json.find_first_of(",}", start);
            if (end == std::string::npos)
                end = json.size();
            std::string value = json.substr(start, end - start);
            if (!value.empty() && value.front() == '"' && value.back() == '"')
                value = value.substr(1, value.size() - 2);
            return value;
        }

        // Try unquoted version: id:value
        pos = json.find("id:");
        if (pos == std::string::npos)
            return "null";
        pos = json.find(':', pos);
        if (pos == std::string::npos)
            return "null";
        auto start = json.find_first_not_of(" \t\n\r", pos + 1);
        if (start == std::string::npos)
            return "null";
        auto end = json.find_first_of(",}", start);
        if (end == std::string::npos)
            end = json.size();
        return json.substr(start, end - start);
    }

    std::string RpcServer::jsonExtractMethod(const std::string &json)
    {
        return jsonExtractStringParam(json, "method");
    }

    std::string RpcServer::jsonExtractParamByIndex(const std::string &json, std::size_t index)
    {
        auto pos = json.find("\"params\"");
        if (pos == std::string::npos)
            return "";
        pos = json.find('[', pos);
        if (pos == std::string::npos)
            return "";
        // naive: iterate quoted strings
        std::size_t currentIndex = 0;
        while (true)
        {
            pos = json.find('"', pos + 1);
            if (pos == std::string::npos)
                return "";
            auto end = json.find('"', pos + 1);
            if (end == std::string::npos)
                return "";
            if (currentIndex == index)
            {
                return json.substr(pos + 1, end - pos - 1);
            }
            currentIndex++;
            pos = end + 1;
        }
    }

    // Stub helper functions for seeder methods (currently not used)
    static std::map<std::string, std::string> jsonExtractObjectParams(const std::string &json)
    {
        return {}; // Not implemented; seeder methods disabled
    }

    static std::string strip0x(const std::string &s)
    {
        if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0)
        {
            return s.substr(2);
        }
        return s;
    }

    static bool hasParam(const std::string &json, std::size_t index)
    {
        // Simple check: count commas in params array
        auto pos = json.find("\"params\"");
        if (pos == std::string::npos)
            return false;
        auto arrStart = json.find('[', pos);
        if (arrStart == std::string::npos)
            return false;
        auto arrEnd = json.find(']', arrStart);
        if (arrEnd == std::string::npos)
            return false;

        std::size_t count = 0;
        for (auto i = arrStart + 1; i < arrEnd; ++i)
        {
            if (json[i] == ',')
                count++;
        }
        return count >= index;
    }

} // namespace gambit
