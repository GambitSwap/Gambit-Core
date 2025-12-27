#include "gambit/block.hpp"
#include <chrono>
#include <stdexcept>

namespace gambit {

Block::Block(std::uint64_t idx,
             const std::string& prev,
             const std::string& before,
             const std::string& after,
             const std::string& txRoot_,
             const ZkProof& proof_)
    : index(idx),
      prevHash(prev),
      stateBefore(before),
      stateAfter(after),
      txRoot(txRoot_),
      proof(proof_)
{
    timestamp = static_cast<std::uint64_t>(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    hash = computeHash();
}

std::string Block::computeHash() const {
    std::string concat =
        std::to_string(index) + "|" +
        prevHash + "|" +
        stateBefore + "|" +
        stateAfter + "|" +
        txRoot + "|" +
        receiptsRoot + "|" +
        proof.commitment + "|" +
        std::to_string(timestamp);

    return gambit::toHex(keccak256(concat));
}

std::string Block::toHex() const {
    Bytes enc = rlpEncode();
    return "0x" + gambit::toHex(enc);
}

Block Block::fromHex(const std::string& hex) {
    std::string h = hex;
    if (h.rfind("0x", 0) == 0 || h.rfind("0X", 0) == 0) {
        h = h.substr(2);
    }

    Bytes raw = gambit::fromHex(h);
    auto root = rlp::decode(raw);

    if (!root.isList || root.list.size() < 10) {
        throw std::runtime_error("Block::fromHex: invalid RLP block");
    }

    auto& L = root.list;

    auto toUint = [](const Bytes& b) -> std::uint64_t {
        std::uint64_t v = 0;
        for (std::uint8_t c : b) {
            v = (v << 8) | c;
        }
        return v;
    };

    Block b;

    b.index       = toUint(L[0].bytes);
    b.prevHash    = std::string(L[1].bytes.begin(), L[1].bytes.end());
    b.stateBefore = std::string(L[2].bytes.begin(), L[2].bytes.end());
    b.stateAfter  = std::string(L[3].bytes.begin(), L[3].bytes.end());
    b.txRoot      = std::string(L[4].bytes.begin(), L[4].bytes.end());
    b.proof.proof      = std::string(L[5].bytes.begin(), L[5].bytes.end());
    b.proof.commitment = std::string(L[6].bytes.begin(), L[6].bytes.end());
    b.timestamp  = toUint(L[7].bytes);
    b.hash       = std::string(L[8].bytes.begin(), L[8].bytes.end());

    // tx list
    const auto& txListNode = L[9];
    if (!txListNode.isList) {
        throw std::runtime_error("Block::fromHex: tx list not a list");
    }

    for (const auto& item : txListNode.list) {
        // item.bytes is raw RLP for a single signed tx
        std::string txHex = "0x" + gambit::toHex(item.bytes);
        Transaction tx = Transaction::fromHex(txHex);
        b.transactions.push_back(std::move(tx));
    }

    return b;
}


Bytes Block::rlpEncode() const {
    using namespace rlp;

    std::vector<Bytes> fields;
    fields.push_back(encodeUint(index));
    fields.push_back(encodeString(prevHash));
    fields.push_back(encodeString(stateBefore));
    fields.push_back(encodeString(stateAfter));
    fields.push_back(encodeString(txRoot));
    fields.push_back(encodeString(proof.proof));
    fields.push_back(encodeString(proof.commitment));
    fields.push_back(encodeUint(timestamp));
    fields.push_back(encodeString(hash));

    std::vector<Bytes> txItems;
    for (const auto& tx : transactions) {
        txItems.push_back(tx.rlpEncodeSigned());
    }
    fields.push_back(encodeList(txItems));

    fields.push_back(encodeBytes(Bytes(logsBloom.bits.begin(), logsBloom.bits.end())));

    std::vector<Bytes> rcItems;
    for (const auto& r : receipts) {
        rcItems.push_back(r.rlpEncode());
    }
    fields.push_back(encodeList(rcItems));

    return encodeList(fields);
}

Block Block::rlpDecode(const Bytes& raw) {
    auto root = rlp::decode(raw);
    if (!root.isList || root.list.size() < 10)
        throw std::runtime_error("Invalid RLP block");

    auto& L = root.list;

    auto toUint = [](const Bytes& b) -> uint64_t {
        uint64_t v = 0;
        for (uint8_t c : b) v = (v << 8) | c;
        return v;
    };

    Block b;
    b.index       = toUint(L[0].bytes);
    b.prevHash    = std::string(L[1].bytes.begin(), L[1].bytes.end());
    b.stateBefore = std::string(L[2].bytes.begin(), L[2].bytes.end());
    b.stateAfter  = std::string(L[3].bytes.begin(), L[3].bytes.end());
    b.txRoot      = std::string(L[4].bytes.begin(), L[4].bytes.end());
    b.proof.proof      = std::string(L[5].bytes.begin(), L[5].bytes.end());
    b.proof.commitment = std::string(L[6].bytes.begin(), L[6].bytes.end());
    b.timestamp = toUint(L[7].bytes);
    b.hash      = std::string(L[8].bytes.begin(), L[8].bytes.end());

    // tx list
    auto& txListNode = L[9];
    if (!txListNode.isList) throw std::runtime_error("Block txs must be list");

    for (const auto& item : txListNode.list) {
        Transaction tx = Transaction::fromHex("0x" + gambit::toHex(item.bytes));
        b.transactions.push_back(std::move(tx));
    }

    return b;
}


} // namespace gambit
