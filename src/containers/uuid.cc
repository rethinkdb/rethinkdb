// Copyright 2010-2012 RethinkDB, all rights reserved.

#include <unistd.h>
#include "containers/uuid.hpp"

#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

#include "errors.hpp"
#include "containers/printf_buffer.hpp"

#include "utils.hpp"
#include "thread_local.hpp"

// We keep the sha1 functions in this .cc file to avoid encouraging others from using it.
namespace sha1 {
// hash is a pointer to 20-byte array, unfortunately.
void calc(const void* src, const int bytelength, unsigned char* hash);
}

static const char *const magic_unset_uuid = "UNSET_UUID_____";
uuid_t::uuid_t() {
    rassert(strlen(magic_unset_uuid) == kStaticSize-1);
    memcpy(data_, magic_unset_uuid, kStaticSize);
}
bool uuid_t::is_unset() const {
    return !memcmp(data_, magic_unset_uuid, kStaticSize);
}

bool uuid_t::is_nil() const {
    for (size_t i = 0; i < kStaticSize; ++i) {
        if (data_[i] != 0) return false;
    }
    return true;
}

bool operator==(const uuid_t& x, const uuid_t& y) {
    return memcmp(x.data(), y.data(), uuid_t::static_size()) == 0;
}

bool operator<(const uuid_t& x, const uuid_t& y) {
    return memcmp(x.data(), y.data(), uuid_t::static_size()) < 0;
}

static __thread bool next_uuid_initialized = false;
static __thread uint8_t next_uuid[uuid_t::kStaticSize];

uuid_t get_and_increment_uuid() {
    uuid_t result;

    // Copy over the next_uuid buffer
    uint8_t *result_buffer = result.data();
    memcpy(result_buffer, next_uuid, uuid_t::static_size());

    // Increment the next_uuid buffer
    bool carry = true;
    for (size_t i = uuid_t::static_size(); carry && i > 0; --i) {
        next_uuid[i - 1] = next_uuid[i - 1] + 1;
        carry = (next_uuid[i - 1] == 0);
    }

    return result;
}

// TODO(sam):  Make sure this isn't messed up somehow.
void hash_uuid(uuid_t *uuid) {
    CT_ASSERT(20 >= uuid_t::kStaticSize);
    uint8_t output_buffer[20];

    sha1::calc(uuid->data(), uuid_t::static_size(), output_buffer);

    // Set some bits to obey standard for version 4 UUIDs.
    output_buffer[6] = ((output_buffer[6] & 0x0f) | 0x40);
    output_buffer[8] = ((output_buffer[8] & 0x3f) | 0x80);

    // Copy the beginning of the hash into our uuid
    memcpy(uuid->data(), output_buffer, uuid_t::static_size());
}

void initialize_dev_random_uuid() {
    int random_fd = open("/dev/urandom", O_RDONLY);
    guarantee(random_fd != -1);
    ssize_t readres = read(random_fd, next_uuid, uuid_t::static_size());
    guarantee(readres == static_cast<ssize_t>(uuid_t::static_size()));
    close(random_fd);
}

uuid_t generate_uuid() {
    if (!next_uuid_initialized) {
        initialize_dev_random_uuid();
        next_uuid_initialized = true;
    }
    uuid_t result = get_and_increment_uuid();
    hash_uuid(&result);
    return result;
}

uuid_t nil_uuid() {
    uuid_t ret;
    memset(ret.data(), 0, uuid_t::static_size());
    return ret;
}

void debug_print(append_only_printf_buffer_t *buf, const uuid_t& id) {
    buf->appendf("%s", uuid_to_str(id).c_str());
}

void push_hex(std::string *s, uint8_t byte) {
    const char *buf = "0123456789abcdef";
    s->push_back(buf[byte >> 4]);
    s->push_back(buf[byte & 0x0f]);
}

std::string uuid_to_str(uuid_t id) {
    const uint8_t *data = id.data();

    std::string ret;
    ret.reserve(uuid_t::kStringSize);
    size_t i = 0;
    for (; i < 4; ++i) {
        push_hex(&ret, data[i]);
    }
    ret.push_back('-');
    for (; i < 6; ++i) {
        push_hex(&ret, data[i]);
    }
    ret.push_back('-');
    for (; i < 8; ++i) {
        push_hex(&ret, data[i]);
    }
    ret.push_back('-');
    for (; i < 10; ++i) {
        push_hex(&ret, data[i]);
    }
    ret.push_back('-');
    CT_ASSERT(uuid_t::kStaticSize == 16);  // This code just feels this assertion in its bones.
    for (; i < uuid_t::kStaticSize; ++i) {
        push_hex(&ret, data[i]);
    }

    return ret;
}

uuid_t str_to_uuid(const std::string &uuid) {
    uuid_t ret;
    if (str_to_uuid(uuid, &ret)) {
        return ret;
    } else {
        throw std::runtime_error("invalid uuid");  // Sigh.
    }
}

MUST_USE bool from_hexdigit(int ch, int *out) {
    if (isdigit(ch)) {
        *out = ch - '0';
        return true;
    }
    ch = tolower(ch);
    if ('a' <= ch && ch <= 'f') {  // Death to EBCDIC.
        *out = 10 + (ch - 'a');
        return true;
    }
    return false;
}

MUST_USE bool str_to_uuid(const std::string &str, uuid_t *uuid) {
    if (str.size() != uuid_t::kStaticSize * 2 + 4) {
        return false;
    }

    uint8_t *data = uuid->data();

    size_t j = 0;
    for (size_t i = 0; i < uuid_t::kStaticSize; ++i) {
        // Uh oh.. a for/switch loop!
        switch (i) {
        case 4:
        case 6:
        case 8:
        case 10:
            rassert(j < uuid_t::kStringSize);
            if (str[j] != '-') {
                return false;
            }
            ++j;
            // fall through
        default: {
            rassert(j < uuid_t::kStringSize);
            int high;
            if (!from_hexdigit(str[j], &high)) {
                return false;
            }
            ++j;
            rassert(j < uuid_t::kStringSize);
            int low;
            if (!from_hexdigit(str[j], &low)) {
                return false;
            }
            ++j;
            data[i] = ((high << 4) | low);
        } break;
        }
    }

    rassert(j == uuid_t::kStringSize);
    return true;
}

bool is_uuid(const std::string& str) {
    try {
        str_to_uuid(str);
    } catch (...) {
        return false;
    }
    return true;
}


/*
 For part of this file below:

 Copyright (c) 2011, Micael Hildenborg
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Micael Hildenborg nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY Micael Hildenborg ''AS IS'' AND ANY
 EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL Micael Hildenborg BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
namespace sha1
{
namespace // local
{
// Rotate an integer value to left.
inline unsigned int rol(const unsigned int value, const unsigned int steps)
{
    return ((value << steps) | (value >> (32 - steps)));
}

// Sets the first 16 integers in the buffert to zero.
// Used for clearing the W buffert.
inline void clearWBuffert(unsigned int* buffert)
{
    for (int pos = 16; --pos >= 0;)
    {
        buffert[pos] = 0;
    }
}

void innerHash(unsigned int* result, unsigned int* w)
{
    unsigned int a = result[0];
    unsigned int b = result[1];
    unsigned int c = result[2];
    unsigned int d = result[3];
    unsigned int e = result[4];

    int round = 0;

    #define sha1macro(func, val) \
                { \
        const unsigned int t = rol(a, 5) + (func) + e + val + w[round]; \
                        e = d; \
                        d = c; \
                        c = rol(b, 30); \
                        b = a; \
                        a = t; \
                }

    while (round < 16)
    {
        sha1macro((b & c) | (~b & d), 0x5a827999)
        ++round;
    }
    while (round < 20)
    {
        w[round] = rol((w[round - 3] ^ w[round - 8] ^ w[round - 14] ^ w[round - 16]), 1);
        sha1macro((b & c) | (~b & d), 0x5a827999)
        ++round;
    }
    while (round < 40)
    {
        w[round] = rol((w[round - 3] ^ w[round - 8] ^ w[round - 14] ^ w[round - 16]), 1);
        sha1macro(b ^ c ^ d, 0x6ed9eba1)
        ++round;
    }
    while (round < 60)
    {
        w[round] = rol((w[round - 3] ^ w[round - 8] ^ w[round - 14] ^ w[round - 16]), 1);
        sha1macro((b & c) | (b & d) | (c & d), 0x8f1bbcdc)
        ++round;
    }
    while (round < 80)
    {
        w[round] = rol((w[round - 3] ^ w[round - 8] ^ w[round - 14] ^ w[round - 16]), 1);
        sha1macro(b ^ c ^ d, 0xca62c1d6)
        ++round;
    }

    #undef sha1macro

    result[0] += a;
    result[1] += b;
    result[2] += c;
    result[3] += d;
    result[4] += e;
}
} // namespace

// hash is a pointer to a 20-byte array.
void calc(const void* src, const int bytelength, unsigned char* hash)
{
    // Init the result array.
    unsigned int result[5] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0 };

    // Cast the void src pointer to be the byte array we can work with.
    const unsigned char* sarray = (const unsigned char*) src;

    // The reusable round buffer
    unsigned int w[80];

    // Loop through all complete 64byte blocks.
    const int endOfFullBlocks = bytelength - 64;
    int endCurrentBlock;
    int currentBlock = 0;

    while (currentBlock <= endOfFullBlocks)
    {
        endCurrentBlock = currentBlock + 64;

        // Init the round buffer with the 64 byte block data.
        for (int roundPos = 0; currentBlock < endCurrentBlock; currentBlock += 4)
        {
            // This line will swap endian on big endian and keep endian on little endian.
            w[roundPos++] = (unsigned int) sarray[currentBlock + 3]
                    | (((unsigned int) sarray[currentBlock + 2]) << 8)
                    | (((unsigned int) sarray[currentBlock + 1]) << 16)
                    | (((unsigned int) sarray[currentBlock]) << 24);
        }
        innerHash(result, w);
    }

    // Handle the last and not full 64 byte block if existing.
    endCurrentBlock = bytelength - currentBlock;
    clearWBuffert(w);
    int lastBlockBytes = 0;
    for (; lastBlockBytes < endCurrentBlock; ++lastBlockBytes)
    {
        w[lastBlockBytes >> 2] |= (unsigned int) sarray[lastBlockBytes + currentBlock] << ((3 - (lastBlockBytes & 3)) << 3);
    }
    w[lastBlockBytes >> 2] |= 0x80 << ((3 - (lastBlockBytes & 3)) << 3);
    if (endCurrentBlock >= 56)
    {
        innerHash(result, w);
        clearWBuffert(w);
    }
    w[15] = bytelength << 3;
    innerHash(result, w);

    // Store hash in result pointer, and make sure we get in in the correct order on both endian models.
    for (int hashByte = 20; --hashByte >= 0;)
    {
        hash[hashByte] = (result[hashByte >> 2] >> (((3 - hashByte) & 0x3) << 3)) & 0xff;
    }
}
} // namespace sha1
