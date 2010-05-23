/*
    This file is part of the Meique project
    Copyright (C) 2010 Hugo Parente Lima <hugo.pl@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "filehash.h"
#include "logger.h"
#include <fstream>
#include <sstream>
#include <cstring>

static std::string computeMd5(const std::string& fileName);

std::string getFileHash(const std::string& fileName)
{
    return computeMd5(fileName);
}

/*
    The following source code used to compute MD5 is based on:

    MD5C.C - RSA Data Security, Inc., MD5 message-digest algorithm
    MDDRIVER.C - test driver for MD2, MD4 and MD5

    Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
    rights reserved.

    License to copy and use this software is granted provided that it
    is identified as the "RSA Data Security, Inc. MD5 Message-Digest
    Algorithm" in all material mentioning or referencing this software
    or this function.

    License is also granted to make and use derivative works provided
    that such works are identified as "derived from the RSA Data
    Security, Inc. MD5 Message-Digest Algorithm" in all material
    mentioning or referencing the derived work.

    RSA Data Security, Inc. makes no representations concerning either
    the merchantability of this software or the suitability of this
    software for any particular purpose. It is provided "as is"
    without express or implied warranty of any kind.

    These notices must be retained in any copies of any part of this
    documentation and/or software.

*/

// Constants for MD5Transform routine.
// Although we could use C++ style constants, defines are actually better,
// since they let us easily evade scope clashes.

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

typedef unsigned       int uint4; // assumes integer is 4 words long
typedef unsigned short int uint2; // assumes short integer is 2 words long
typedef unsigned      char uint1; // assumes char is 1 word long

struct MD5State
{
    uint4 state[4];
    uint4 count[2];     // number of *bits*, mod 2^64
    uint1 buffer[64];   // input buffer
    uint1 digest[16];   // pointer to mDigest!
};

// ROTATE_LEFT rotates x left n bits.
inline unsigned int rotateLeft(uint4 x, uint4 n)
{
    return (x << n) | (x >> (32-n))  ;
}

// F, G, H and I are basic MD5 functions.

inline unsigned int F(uint4 x, uint4 y, uint4 z)
{
    return (x & y) | (~x & z);
}

inline unsigned int G(uint4 x, uint4 y, uint4 z)
{
    return (x & z) | (y & ~z);
}

inline unsigned int H(uint4 x, uint4 y, uint4 z)
{
    return x ^ y ^ z;
}

inline unsigned int I(uint4 x, uint4 y, uint4 z)
{
    return y ^ (x | ~z);
}


// FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
// Rotation is separate from addition to prevent recomputation.

static void FF(uint4& a, uint4 b, uint4 c, uint4 d, uint4 x, uint4  s, uint4 ac)
{
    a += F(b, c, d) + x + ac;
    a = rotateLeft (a, s) +b;
}

static void GG(uint4& a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac)
{
    a += G(b, c, d) + x + ac;
    a = rotateLeft (a, s) +b;
}

static void HH(uint4& a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac)
{
    a += H(b, c, d) + x + ac;
    a = rotateLeft (a, s) +b;
}

static void II(uint4& a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac)
{
    a += I(b, c, d) + x + ac;
    a = rotateLeft (a, s) +b;
}

// Decodes input (unsigned char) into output (UINT4). Assumes len is
// a multiple of 4.
static void decode(uint4* output, uint1* input, uint4 len)
{
    unsigned int i, j;
    for (i = 0, j = 0; j < len; i++, j += 4)
        output[i] = ((uint4)input[j]) | (((uint4)input[j+1]) << 8) |
        (((uint4)input[j+2]) << 16) | (((uint4)input[j+3]) << 24);
}

// MD5 basic transformation. Transforms state based on block.
static void transform(MD5State* state, uint1 block[64])
{
    uint4 a = state->state[0];
    uint4 b = state->state[1];
    uint4 c = state->state[2];
    uint4 d = state->state[3];
    uint4 x[16];

    decode(x, block, 64);

    /* Round 1 */
    FF (a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
    FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
    FF (c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
    FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
    FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
    FF (d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
    FF (c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
    FF (b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
    FF (a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
    FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
    FF (c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
    FF (b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
    FF (a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
    FF (d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
    FF (c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
    FF (b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

    /* Round 2 */
    GG (a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
    GG (d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
    GG (c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
    GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
    GG (a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
    GG (d, a, b, c, x[10], S22,  0x2441453); /* 22 */
    GG (c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
    GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
    GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
    GG (d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
    GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
    GG (b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
    GG (a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
    GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
    GG (c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
    GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

    /* Round 3 */
    HH (a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
    HH (d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
    HH (c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
    HH (b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
    HH (a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
    HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
    HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
    HH (b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
    HH (a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
    HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
    HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
    HH (b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
    HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
    HH (d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
    HH (c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
    HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

    /* Round 4 */
    II (a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
    II (d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
    II (c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
    II (b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
    II (a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
    II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
    II (c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
    II (b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
    II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
    II (d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
    II (c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
    II (b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
    II (a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
    II (d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
    II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
    II (b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

    state->state[0] += a;
    state->state[1] += b;
    state->state[2] += c;
    state->state[3] += d;
}

static void update(MD5State* state, uint1* input, uint4 inputLength)
{

    uint4 inputIndex, bufferIndex;
    uint4 bufferSpace;                // how much space is left in buffer

    // Compute number of bytes mod 64
    bufferIndex = (uint4)((state->count[0] >> 3) & 0x3F);

    // Update number of bits
    if ((state->count[0] += ((uint4) inputLength << 3))<((uint4) inputLength << 3) )
        state->count[1]++;

    state->count[1] += ((uint4)inputLength >> 29);
    bufferSpace = 64 - bufferIndex;  // how much space is left in buffer

    // Transform as many times as possible.
    if (inputLength >= bufferSpace) { // ie. we have enough to fill the buffer
        // fill the rest of the buffer and transform
        memcpy(state->buffer + bufferIndex, input, bufferSpace);
        transform(state, state->buffer);

        // now, transform each 64-byte piece of the input, bypassing the buffer
        for (inputIndex = bufferSpace; inputIndex + 63 < inputLength;
        inputIndex += 64)
        transform(state, input+inputIndex);

        bufferIndex = 0;  // so we can buffer remaining
    } else {
        inputIndex=0;     // so we can buffer the whole input
    }

    // and here we do the buffering:
    memcpy(state->buffer+bufferIndex, input+inputIndex, inputLength-inputIndex);
}

// Encodes input (UINT4) into output (unsigned char). Assumes len is
// a multiple of 4.
static void encode(uint1* output, uint4* input, uint4 len)
{
    unsigned int i, j;
    for (i = 0, j = 0; j < len; i++, j += 4) {
        output[j]   = (uint1)  (input[i] & 0xff);
        output[j+1] = (uint1) ((input[i] >> 8) & 0xff);
        output[j+2] = (uint1) ((input[i] >> 16) & 0xff);
        output[j+3] = (uint1) ((input[i] >> 24) & 0xff);
    }
}

// MD5 finalization. Ends an MD5 message-digest operation, writing the
// the message digest and zeroizing the context.
static void finalize(MD5State* state)
{
    unsigned char bits[8];
    unsigned int index, padLen;
    static uint1 PADDING[64]={
        0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    // Save number of bits
    encode(bits, state->count, 8);

    // Pad out to 56 mod 64.
    index = (uint4) ((state->count[0] >> 3) & 0x3f);
    padLen = (index < 56) ? (56 - index) : (120 - index);
    update(state, PADDING, padLen);

    // Append length (before padding)
    update(state, bits, 8);

    // Store state in digest
    encode((uint1*)state->digest, state->state, 16);
}

std::string computeMd5(const std::string& fileName)
{
    std::ifstream file(fileName.c_str());
    if (!file)
        return std::string();
    MD5State state;
    // Nothing counted, so count=0
    state.count[0] = 0;
    state.count[1] = 0;
    // Load magic initialization constants.
    state.state[0] = 0x67452301;
    state.state[1] = 0xefcdab89;
    state.state[2] = 0x98badcfe;
    state.state[3] = 0x10325476;
    std::memset(state.digest, 0, sizeof(state.digest));

    const int BUFFER_SIZE = 1024;
    unsigned char buffer[BUFFER_SIZE];

    while(file.good()) {
        file.read((char*)buffer, BUFFER_SIZE);
        update(&state, buffer, file.gcount());
    }
    finalize(&state);
    std::ostringstream out;
    out.fill('0');
    for (int i = 0; i < 16; ++i) {
        out.width(2);
        out << std::hex << std::uppercase << std::internal << (unsigned int)state.digest[i];
    }
    return out.str();
}
