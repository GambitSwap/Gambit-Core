// Keccak-256 implementation based on XKCP/tiny_sha3
// Simplified single-file implementation for Gambit

#include "keccak.hpp"
#include <cstring>

namespace tinykeccak {

// Keccak round constants
static const uint64_t keccakf_rndc[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL,
    0x800000000000808aULL, 0x8000000080008000ULL,
    0x000000000000808bULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL,
    0x000000000000008aULL, 0x0000000000000088ULL,
    0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL,
    0x8000000000008089ULL, 0x8000000000008003ULL,
    0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800aULL, 0x800000008000000aULL,
    0x8000000080008081ULL, 0x8000000000008080ULL,
    0x0000000080000001ULL, 0x8000000080008008ULL
};

// Rotation offsets
static const int keccakf_rotc[24] = {
    1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14,
    27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44
};

static const int keccakf_piln[24] = {
    10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4,
    15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1
};

static inline uint64_t ROTL64(uint64_t x, int y) {
    return (x << y) | (x >> (64 - y));
}

static void keccakf(uint64_t st[25]) {
    uint64_t t, bc[5];

    for (int r = 0; r < 24; r++) {
        // Theta
        for (int i = 0; i < 5; i++)
            bc[i] = st[i] ^ st[i + 5] ^ st[i + 10] ^ st[i + 15] ^ st[i + 20];

        for (int i = 0; i < 5; i++) {
            t = bc[(i + 4) % 5] ^ ROTL64(bc[(i + 1) % 5], 1);
            for (int j = 0; j < 25; j += 5)
                st[j + i] ^= t;
        }

        // Rho Pi
        t = st[1];
        for (int i = 0; i < 24; i++) {
            int j = keccakf_piln[i];
            bc[0] = st[j];
            st[j] = ROTL64(t, keccakf_rotc[i]);
            t = bc[0];
        }

        // Chi
        for (int j = 0; j < 25; j += 5) {
            for (int i = 0; i < 5; i++)
                bc[i] = st[j + i];
            for (int i = 0; i < 5; i++)
                st[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
        }

        // Iota
        st[0] ^= keccakf_rndc[r];
    }
}

void keccak_256(const uint8_t* input, size_t inlen, uint8_t* output) {
    uint64_t st[25] = {0};
    
    const size_t rsiz = 136;  // rate for Keccak-256 (1088 bits = 136 bytes)
    const size_t rsizw = rsiz / 8;
    
    size_t pt = 0;
    
    // Absorb input
    while (inlen >= rsiz) {
        for (size_t i = 0; i < rsizw; i++) {
            uint64_t t = 0;
            for (int j = 0; j < 8; j++) {
                t |= static_cast<uint64_t>(input[i * 8 + j]) << (8 * j);
            }
            st[i] ^= t;
        }
        keccakf(st);
        input += rsiz;
        inlen -= rsiz;
    }
    
    // Absorb remaining + padding
    uint8_t temp[144] = {0};  // max rsiz + some padding room
    std::memcpy(temp, input, inlen);
    temp[inlen] = 0x01;  // Keccak padding (not SHA3 which uses 0x06)
    temp[rsiz - 1] |= 0x80;
    
    for (size_t i = 0; i < rsizw; i++) {
        uint64_t t = 0;
        for (int j = 0; j < 8; j++) {
            t |= static_cast<uint64_t>(temp[i * 8 + j]) << (8 * j);
        }
        st[i] ^= t;
    }
    keccakf(st);
    
    // Squeeze output (32 bytes for Keccak-256)
    for (size_t i = 0; i < 4; i++) {
        for (int j = 0; j < 8; j++) {
            output[i * 8 + j] = static_cast<uint8_t>(st[i] >> (8 * j));
        }
    }
}

} // namespace tinykeccak

