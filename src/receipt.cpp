#include "gambit/receipt.hpp"

namespace gambit {

Bytes Receipt::rlpEncode() const {
    using namespace rlp;

    std::vector<Bytes> fields;
    fields.push_back(encodeUint(status ? 1 : 0));
    fields.push_back(encodeUint(cumulativeGasUsed));

    // logs as list (we keep it empty for now)
    std::vector<Bytes> logItems;
    for (const auto& log : logs) {
        std::vector<Bytes> lf;
        lf.push_back(encodeBytes(Bytes(log.address.bytes().begin(), log.address.bytes().end())));
        std::vector<Bytes> topicBytes;
        for (const auto& t : log.topics) {
            topicBytes.push_back(encodeBytes(fromHex(t)));
        }
        lf.push_back(encodeList(topicBytes));
        lf.push_back(encodeBytes(log.data));
        logItems.push_back(encodeList(lf));
    }
    fields.push_back(encodeList(logItems));

    return encodeList(fields);
}

} // namespace gambit
