#pragma once
#include <memory>
#include <vector>
#include <array>
#include <string>
#include <cstdint>
#include <optional>

#include "gambit/hash.hpp"
#include "gambit/rlp.hpp"

namespace gambit {

class MptTrie {
public:
    MptTrie();

    // key: arbitrary bytes (we'll use 20-byte address)
    void put(const Bytes& key, const Bytes& value);

    // Returns empty optional if not found
    std::optional<Bytes> get(const Bytes& key) const;

    // Root hash (Keccak-256 of RLP(root node))
    std::string rootHash() const;

private:
    struct Node;
    using NodePtr = std::shared_ptr<Node>;

    struct Node {
        // 0..15 children + optional value
        std::array<NodePtr, 16> children{};
        std::optional<Bytes> value;
    };

    NodePtr root_;

    static std::vector<uint8_t> toNibbles(const Bytes& key);
    static Bytes encodeNode(const NodePtr& node);
    static Bytes encodeNodeValue(const NodePtr& node);
};

} // namespace gambit
