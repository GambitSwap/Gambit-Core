#include "gambit/rpc_server.hpp"
#include "gambit/hash.hpp"
#include "gambit/address.hpp"
#include "gambit/transaction.hpp"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

namespace gambit
{

    RpcServer::RpcServer(Blockchain &chain, std::uint16_t port)
        : chain_(chain), port_(port) {}

    void RpcServer::start()
    {
        running_ = true;

        listenSock_ = socket(AF_INET, SOCK_STREAM, 0);

        int opt = 1;
        setsockopt(listenSock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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
        ssize_t n = recv(clientFd, buf, BUF_SIZE, 0);
        if (n <= 0)
        {
            close(clientFd);
            return;
        }

        std::string req(buf, buf + n);
        std::string resp = handleRequest(req);

        send(clientFd, resp.data(), resp.size(), 0);
        close(clientFd);
    }

    std::string RpcServer::handleRequest(const std::string &httpReq)
    {
        // Very naive HTTP parsing: look for body after double CRLF
        auto pos = httpReq.find("\r\n\r\n");
        if (pos == std::string::npos)
        {
            return httpResponse(R"({"jsonrpc":"2.0","error":{"code":-32700,"message":"Parse error"},"id":null})");
        }

        std::string body = httpReq.substr(pos + 4);
        std::string jsonResp = handleJsonRpc(body);
        return httpResponse(jsonResp);
    }

    std::string RpcServer::handleJsonRpc(const std::string &json)
    {
        std::string id = jsonExtractId(json);
        std::string method = jsonExtractMethod(json);

        if (method == "eth_blockNumber")
        {
            return handle_blockNumber(id);
        }
        else if (method == "eth_getBalance")
        {
            std::string addr = jsonExtractParamByIndex(json, 0);
            return handle_getBalance(id, addr);
        }
        else if (method == "eth_sendRawTransaction")
        {
            std::string txHex = jsonExtractParamByIndex(json, 0);
            return handle_sendRawTransaction(id, txHex);
        }
        else if (method == "net_version")
        {
            // chainId as decimal string
            std::string chainIdStr = std::to_string(chain_.chain().front().index == 0 ? 1337 : 1);
            return jsonResult(id, "\"" + chainIdStr + "\"");
        }
        else if (method == "eth_getBlockByNumber")
        {
            std::string num = jsonExtractParamByIndex(json, 0);
            return handle_getBlockByNumber(id, num);
        }
        else if (method == "eth_getBlockByHash")
        {
            std::string h = jsonExtractParamByIndex(json, 0);
            return handle_getBlockByHash(id, h);
        }
        else if (method == "eth_getTransactionByHash")
        {
            std::string h = jsonExtractParamByIndex(json, 0);
            return handle_getTransactionByHash(id, h);
        }
        else if (method == "eth_getTransactionCount")
        {
            std::string addr = jsonExtractParamByIndex(json, 0);
            return handle_getTransactionCount(id, addr);
        }
        else if (method == "miner_start")
        {
            miner_.start();
            return jsonResult(id, "\"ok\"");
        }
        else if (method == "miner_stop")
        {
            miner_.stop();
            return jsonResult(id, "\"ok\"");
        }
        else if (method == "miner_setInterval")
        {
            std::string ms = jsonExtractParamByIndex(json, 0);
            miner_.setInterval(std::chrono::milliseconds(std::stoull(ms)));
            return jsonResult(id, "\"ok\"");
        }
        else if (method == "eth_getWork")
        {
            Block b = miner_.getWork();
            return jsonResult(id, "\"" + b.toHex() + "\"");
        }
        else if (method == "eth_submitWork")
        {
            std::string blockHex = jsonExtractParamByIndex(json, 0);
            Block b = Block::fromHex(blockHex);
            bool ok = miner_.submitWork(b);
            return jsonResult(id, ok ? "\"ok\"" : "\"invalid\"");
        }

        return jsonError(id, -32601, "Method not found");
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
        const auto &chain = chain_.chain();
        for (const auto &b : chain)
        {
            if (b.hash == hashHex)
            {
                std::string out = "{"
                                  "\"number\":\"0x" +
                                  toHex(rlp::encodeUint(b.index)) + "\","
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
        }
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
        auto pos = json.find("\"" + key + "\"");
        if (pos == std::string::npos)
            return "";
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

    std::string RpcServer::jsonExtractId(const std::string &json)
    {
        // id can be number or string; we just return raw token
        auto pos = json.find("\"id\"");
        if (pos == std::string::npos)
            return "null";
        pos = json.find(':', pos);
        if (pos == std::string::npos)
            return "null";
        // find end of token (comma or closing brace)
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

} // namespace gambit
