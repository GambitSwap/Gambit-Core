#include "gambit/transaction.hpp"
#include <stdexcept>

namespace gambit
{

    Bytes Transaction::rlpEncodeForSigning() const
    {
        using namespace rlp;

        std::vector<Bytes> fields;

        fields.push_back(encodeUint(nonce));
        fields.push_back(encodeUint(gasPrice));
        fields.push_back(encodeUint(gasLimit));

        if (to.bytes() == std::array<std::uint8_t, 20>{})
        {
            fields.push_back(encodeBytes({})); // contract creation
        }
        else
        {
            fields.push_back(encodeBytes(Bytes(to.bytes().begin(), to.bytes().end())));
        }

        fields.push_back(encodeUint(value));
        fields.push_back(encodeBytes(data));

        // EIP-155: include chainId, 0, 0
        fields.push_back(encodeUint(chainId));
        fields.push_back(encodeUint(0));
        fields.push_back(encodeUint(0));

        return encodeList(fields);
    }

    Bytes Transaction::rlpEncodeSigned() const
    {
        using namespace rlp;

        std::vector<Bytes> fields;

        fields.push_back(encodeUint(nonce));
        fields.push_back(encodeUint(gasPrice));
        fields.push_back(encodeUint(gasLimit));

        if (to.bytes() == std::array<std::uint8_t, 20>{})
        {
            fields.push_back(encodeBytes({}));
        }
        else
        {
            fields.push_back(encodeBytes(Bytes(to.bytes().begin(), to.bytes().end())));
        }

        fields.push_back(encodeUint(value));
        fields.push_back(encodeBytes(data));

        // v = recid + 35 + 2*chainId
        std::uint64_t v = static_cast<std::uint64_t>(sig.v) + 35 + 2 * chainId;

        fields.push_back(encodeUint(v));
        fields.push_back(encodeBytes(sig.r));
        fields.push_back(encodeBytes(sig.s));

        return encodeList(fields);
    }

    Bytes32 Transaction::signingHash() const
    {
        Bytes rlp = rlpEncodeForSigning();
        return keccak256_32(rlp);
    }

    bool Transaction::verifySignature() const
    {
        try
        {
            Bytes32 msgHash = signingHash();
            Address recovered = Keys::recoverAddress(msgHash, sig, chainId);

            // If from is "empty", we just treat recovery as success.
            // If from is set, enforce equality.
            if (!from.isZero() && !(recovered == from))
            {
                return false;
            }
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    std::string Transaction::toHex() const
    {
        Bytes encoded = rlpEncodeSigned();
        return "0x" + gambit::toHex(encoded);
    }

    Transaction Transaction::fromHex(const std::string &hex)
    {
        std::string h = hex;
        if (h.rfind("0x", 0) == 0 || h.rfind("0X", 0) == 0)
        {
            h = h.substr(2);
        }

        Bytes raw = gambit::fromHex(h);
        auto root = rlp::decode(raw);

        if (!root.isList || root.list.size() < 9)
            throw std::runtime_error("Transaction::fromHex: invalid RLP tx");

        auto &L = root.list;

        auto toUint = [](const Bytes &b) -> std::uint64_t
        {
            std::uint64_t v = 0;
            for (std::uint8_t c : b)
                v = (v << 8) | c;
            return v;
        };

        Transaction tx;

        tx.nonce = toUint(L[0].bytes);
        tx.gasPrice = toUint(L[1].bytes);
        tx.gasLimit = toUint(L[2].bytes);

        if (L[3].bytes.empty())
        {
            tx.to = Address(); // contract creation
        }
        else
        {
            tx.to = Address::fromBytes(L[3].bytes);
        }

        tx.value = toUint(L[4].bytes);
        tx.data = L[5].bytes;

        std::uint64_t vFull = toUint(L[6].bytes);
        tx.sig.r = L[7].bytes;
        tx.sig.s = L[8].bytes;

        if (tx.sig.r.size() != 32 || tx.sig.s.size() != 32)
            throw std::runtime_error("Transaction::fromHex: invalid r/s size");

        if (vFull >= 35)
        {
            tx.chainId = (vFull - 35) / 2;
            tx.sig.v = static_cast<std::uint8_t>(vFull); // keep full v for recovery
        }
        else
        {
            tx.chainId = 0;
            tx.sig.v = static_cast<std::uint8_t>(vFull);
        }

        // Compute tx hash
        tx.hash = tx.computeHash();

        // Recover sender
        Bytes32 msgHash = tx.signingHash();
        tx.from = Keys::recoverAddress(msgHash, tx.sig, tx.chainId);

        return tx;
    }

    std::string Transaction::computeHash() const
    {
        Bytes encoded = rlpEncodeSigned();
        Bytes h = keccak256(encoded);
        return "0x" + gambit::toHex(h);
    }

    void Transaction::signWith(const KeyPair &key)
    {
        Bytes32 hash32 = signingHash();
        sig = key.sign(hash32, chainId);

        // Set from from the signing key
        from = key.address();

        hash = computeHash();
    }

} // namespace gambit
