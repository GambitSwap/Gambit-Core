// Gambit/external/tiny-keccak/keccak.hpp
#pragma once
#include <cstddef>
#include <cstdint>

namespace tinykeccak {

// Simple Keccak-256 interface
void keccak_256(const uint8_t* input, size_t inlen, uint8_t* output);

} // namespace tinykeccak


/*
You can wire it to a real implementation later by copying the code from tiny-keccak / tiny_sha3 and adapting.

2) Gambit/external/tiny-keccak/keccak.hpp
You’ll drop a single‑header Keccak implementation here. To avoid pasting a huge external dependency into chat, I’ll describe what you should do:
- Use this well‑known single header:
https://github.com/mjosaarinen/tiny_sha3/blob/master/sha3.h + sha3.c
or
https://github.com/XKCP/XKCP (Keccak Code Package)
For now, assume you place a header‑only Keccak‑256 API here with this interface:


*/