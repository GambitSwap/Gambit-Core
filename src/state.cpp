#include "gambit/state.hpp"
#include <algorithm>
#include <stdexcept>
#include "gambit/rlp.hpp"
#include "gambit/mpt.hpp"

namespace gambit {

State::State(const GenesisConfig& genesis) {
    for (const auto& ga : genesis.premine) {
        accounts_[ga.address.toHex(false)] = Account{ga.balance, 0};
    }
}

Account& State::getOrCreate(const Address& addr) {
    auto key = addr.toHex(false);
    return accounts_[key]; // default-initialized if missing
}

const Account* State::get(const Address& addr) const {
    auto key = addr.toHex(false);
    auto it = accounts_.find(key);
    if (it == accounts_.end()) return nullptr;
    return &it->second;
}

void State::applyTransaction(const Address& from, const Transaction& tx) {
    Account& fromAcc = getOrCreate(from);
    Account& toAcc   = getOrCreate(tx.to);

    if (fromAcc.balance < tx.value) {
        throw std::runtime_error("Insufficient balance");
    }

    fromAcc.balance -= tx.value;
    fromAcc.nonce   += 1;
    toAcc.balance   += tx.value;
}

std::string State::root() const {
    MptTrie trie;

    for (const auto& [addrHex, acc] : accounts_) {
        // Key = 20-byte address
        Bytes key = fromHex(addrHex);

        // Value = RLP[ balance, nonce ]
        std::vector<Bytes> fields;
        fields.push_back(rlp::encodeUint(acc.balance));
        fields.push_back(rlp::encodeUint(acc.nonce));

        Bytes value = rlp::encodeList(fields);

        trie.put(key, value);
    }

    return trie.rootHash();
}

} // namespace gambit
