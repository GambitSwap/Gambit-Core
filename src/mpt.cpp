#include "gambit/mpt.hpp"

namespace gambit {

MptTrie::MptTrie()
    : root_(std::make_shared<Node>()) {}

std::vector<uint8_t> MptTrie::toNibbles(const Bytes& key) {
    std::vector<uint8_t> out;
    out.reserve(key.size() * 2);
    for (auto b : key) {
        out.push_back((b >> 4) & 0x0F);
        out.push_back(b & 0x0F);
    }
    return out;
}

void MptTrie::put(const Bytes& key, const Bytes& value) {
    auto nibbles = toNibbles(key);
    NodePtr node = root_;
    for (auto nib : nibbles) {
        if (!node->children[nib]) {
            node->children[nib] = std::make_shared<Node>();
        }
        node = node->children[nib];
    }
    node->value = value;
}

std::optional<Bytes> MptTrie::get(const Bytes& key) const {
    auto nibbles = toNibbles(key);
    NodePtr node = root_;
    for (auto nib : nibbles) {
        if (!node->children[nib]) {
            return std::nullopt;
        }
        node = node->children[nib];
    }
    if (!node->value) return std::nullopt;
    return node->value;
}

Bytes MptTrie::encodeNodeValue(const NodePtr& node) {
    if (!node->value) {
        // empty => RLP empty string
        return rlp::encodeBytes({});
    }
    return rlp::encodeBytes(*node->value);
}

Bytes MptTrie::encodeNode(const NodePtr& node) {
    std::vector<Bytes> fields;
    fields.reserve(17);

    // 16 children as hashes/embedded
    for (const auto& child : node->children) {
        if (!child) {
            fields.push_back(rlp::encodeBytes({}));  // empty
        } else {
            // For simplicity, always embed full child RLP (no hash-shortcut)
            Bytes enc = encodeNode(child);
            fields.push_back(rlp::encodeBytes(enc));
        }
    }

    // value
    fields.push_back(encodeNodeValue(node));
    return rlp::encodeList(fields);
}

std::string MptTrie::rootHash() const {
    Bytes enc = encodeNode(root_);
    Bytes h = keccak256(enc);
    return "0x" + gambit::toHex(h);
}

} // namespace gambit
