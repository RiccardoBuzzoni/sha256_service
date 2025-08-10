#include "common.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Rotazioni
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define SIGMA0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define SIGMA1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define sigma0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define sigma1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

static uint32_t round_constants[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

void sha256_hash(const unsigned char* data, size_t len, char* output) {
    uint32_t h0 = 0x6a09e667, h1 = 0xbb67ae85, h2 = 0x3c6ef372, h3 = 0xa54ff53a,
             h4 = 0x510e527f, h5 = 0x9b05688c, h6 = 0x1f83d9ab, h7 = 0x5be0cd19;

    // Calcola lunghezza del messaggio con padding
    size_t total_len = len + 9; // +1 per 0x80, +8 per lunghezza
    size_t padded_len = ((total_len + 63) / 64) * 64;
    unsigned char* padded = calloc(padded_len, 1);
    if (!padded) { perror("calloc"); exit(1); }

    memcpy(padded, data, len);
    padded[len] = 0x80; // Aggiungi bit di padding
    uint64_t bit_len = len * 8;
    for (int i = 0; i < 8; i++)
        padded[padded_len - 1 - i] = (bit_len >> (8 * i)) & 0xFF;

    // Processa a blocchi da 64 byte
    for (size_t offset = 0; offset < padded_len; offset += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++) {
            w[i] = (padded[offset + 4*i] << 24) |
                   (padded[offset + 4*i + 1] << 16) |
                   (padded[offset + 4*i + 2] << 8) |
                   (padded[offset + 4*i + 3]);
        }
        for (int i = 16; i < 64; i++) {
            w[i] = sigma1(w[i-2]) + w[i-7] + sigma0(w[i-15]) + w[i-16];
        }

        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4, f = h5, g = h6, h = h7;

        for (int i = 0; i < 64; i++) {
            uint32_t T1 = h + SIGMA1(e) + CH(e, f, g) + round_constants[i] + w[i];
            uint32_t T2 = SIGMA0(a) + MAJ(a, b, c);
            h = g; g = f; f = e; e = d + T1;
            d = c; c = b; b = a; a = T1 + T2;
        }

        h0 += a; h1 += b; h2 += c; h3 += d;
        h4 += e; h5 += f; h6 += g; h7 += h;
    }

    free(padded);

    // Formatta output come stringa esadecimale
    sprintf(output, "%08x%08x%08x%08x%08x%08x%08x%08x",
            h0, h1, h2, h3, h4, h5, h6, h7);
}