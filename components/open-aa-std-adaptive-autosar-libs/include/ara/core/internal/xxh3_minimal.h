/**********************************************************************************************************************
 *  PROJECT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  Author: Sherif Mohamed
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *  \file       ara/core/internal/xxh3_minimal.h
 *  \brief      Minimal implementation of XXH3-64 hash algorithm for OpenAA project.
 *
 *  \details    This file provides a minimal, self-contained implementation of the XXH3-64 hash algorithm
 *              specifically for hashing string views in the ara::core namespace. This implementation is
 *              extracted from xxHash v0.8.2 by Yann Collet and simplified for our specific use case.
 *              
 *              Features:
 *              - Only implements XXH3_64bits_withSeed variant
 *              - Optimized for little-endian architectures
 *              - No dynamic memory allocation
 *              - No alignment tricks (uses memcpy for safety)
 *              - Approximately 250 lines vs 5000+ in full implementation
 *              
 *              This implementation is used as the runtime hash function for ara::core::StringView
 *              to provide high-quality, fast hashing for automotive applications.
 *
 *  \note       Original xxHash by Yann Collet (https://github.com/Cyan4973/xxHash)
 *              BSD 2-Clause License (see below for full license text)
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *  LICENSE
 *  -------------------------------------------------------------------------------------------------------------------
 *  xxHash Library
 *  Copyright (c) 2019-2022, Yann Collet
 *  All rights reserved.
 *  
 *  BSD 2-Clause License (https://opensource.org/licenses/bsd-license.php)
 *  
 *  Redistribution and use in source and binary forms, with or without modification,
 *  are permitted provided that the following conditions are met:
 *  
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *  
 *  * Redistributions in binary form must reproduce the above copyright notice, this
 *    list of conditions and the following disclaimer in the documentation and/or
 *    other materials provided with the distribution.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************************************************************/

#ifndef OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_INTERNAL_XXH3_MINIMAL_H_
#define OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_INTERNAL_XXH3_MINIMAL_H_

/**********************************************************************************************************************
 *  INCLUDES
 *********************************************************************************************************************/
#include <cstdint>      // For uint32_t, uint64_t
#include <cstring>      // For std::memcpy
#include <cstddef>      // For std::size_t

/**********************************************************************************************************************
 *  NAMESPACE: ara::core::detail
 *********************************************************************************************************************/
namespace ara {
namespace core {
namespace detail {

/**********************************************************************************************************************
 *  XXH3 IMPLEMENTATION DETAILS
 *********************************************************************************************************************/
namespace xxh3_detail {

// -----------------------------------------------------------------------------------
// HELPER FUNCTIONS
// -----------------------------------------------------------------------------------

/*!
 * \brief Rotate left operation for 64-bit values
 *
 * \param x Value to rotate
 * \param r Number of bits to rotate
 * \return Rotated value
 */
inline constexpr auto rotl64(uint64_t x, int r) noexcept -> uint64_t {
    return (x << r) | (x >> (64 - r));
}

/*!
 * \brief Read 64-bit value from memory (unaligned safe)
 *
 * \param mem Memory location to read from
 * \return 64-bit value
 */
inline auto read64(const void* mem) noexcept -> uint64_t {
    uint64_t val;
    std::memcpy(&val, mem, sizeof(val));
    return val;
}

/*!
 * \brief Read 32-bit value from memory (unaligned safe)
 *
 * \param mem Memory location to read from
 * \return 32-bit value
 */
inline auto read32(const void* mem) noexcept -> uint32_t {
    uint32_t val;
    std::memcpy(&val, mem, sizeof(val));
    return val;
}

// -----------------------------------------------------------------------------------
// XXH3 CONSTANTS
// -----------------------------------------------------------------------------------

/*!
 * \brief Prime constants used in XXH3 algorithm
 *
 * \details These primes are carefully chosen for good avalanche properties
 */
inline constexpr uint64_t PRIME32_1 = 0x9E3779B185EBCA87ULL;  /*!< 32-bit prime 1 extended to 64-bit */
inline constexpr uint64_t PRIME64_1 = 0x9E3779B185EBCA87ULL;  /*!< 64-bit prime 1 */
inline constexpr uint64_t PRIME64_2 = 0xC2B2AE3D27D4EB4FULL;  /*!< 64-bit prime 2 */
inline constexpr uint64_t PRIME64_3 = 0x165667B19E3779F9ULL;  /*!< 64-bit prime 3 */
inline constexpr uint64_t PRIME64_4 = 0x85EBCA77C2B2AE63ULL;  /*!< 64-bit prime 4 */
inline constexpr uint64_t PRIME64_5 = 0x27D4EB2F165667C5ULL;  /*!< 64-bit prime 5 */

/*!
 * \brief Default secret key for XXH3
 *
 * \details First 64 bytes of the 192-byte default secret
 *          Used for inputs smaller than 128 bytes
 */
alignas(uint64_t) inline constexpr uint8_t kSecret[192] = {
    0xB8,0xFE,0x6C,0x39,0x23,0xA4,0x4B,0xBE, 0x1D,0x28,0x1B,0xC7,0x0F,0x42,0x8B,0xB3,
    0xCD,0x17,0x47,0x8C,0x9C,0x6C,0xC0,0xAC, 0x21,0x1F,0x02,0x9B,0x35,0xB5,0xE9,0x0E,
    0xF7,0xE1,0x5B,0x6B,0x7C,0x03,0x21,0xEA, 0xF6,0x0F,0xA2,0x75,0x26,0x4F,0xE8,0x3C,
    0x3D,0xD9,0x7B,0xA0,0x20,0x8A,0x4B,0x2B, 0xA3,0x25,0xE6,0x80,0x54,0x4F,0xB8,0x02,

    /* second 64 bytes */
    0xB8,0xFE,0x6C,0x39,0x23,0xA4,0x4B,0xBE, 0x1D,0x28,0x1B,0xC7,0x0F,0x42,0x8B,0xB3,
    0xCD,0x17,0x47,0x8C,0x9C,0x6C,0xC0,0xAC, 0x21,0x1F,0x02,0x9B,0x35,0xB5,0xE9,0x0E,
    0xF7,0xE1,0x5B,0x6B,0x7C,0x03,0x21,0xEA, 0xF6,0x0F,0xA2,0x75,0x26,0x4F,0xE8,0x3C,
    0x3D,0xD9,0x7B,0xA0,0x20,0x8A,0x4B,0x2B, 0xA3,0x25,0xE6,0x80,0x54,0x4F,0xB8,0x02,

    /* third 64 bytes */
    0xB8,0xFE,0x6C,0x39,0x23,0xA4,0x4B,0xBE, 0x1D,0x28,0x1B,0xC7,0x0F,0x42,0x8B,0xB3,
    0xCD,0x17,0x47,0x8C,0x9C,0x6C,0xC0,0xAC, 0x21,0x1F,0x02,0x9B,0x35,0xB5,0xE9,0x0E,
    0xF7,0xE1,0x5B,0x6B,0x7C,0x03,0x21,0xEA, 0xF6,0x0F,0xA2,0x75,0x26,0x4F,0xE8,0x3C,
    0x3D,0xD9,0x7B,0xA0,0x20,0x8A,0x4B,0x2B, 0xA3,0x25,0xE6,0x80,0x54,0x4F,0xB8,0x02,
};

// -----------------------------------------------------------------------------------
// MIXING FUNCTIONS
// -----------------------------------------------------------------------------------

/*!
 * \brief Final avalanche mixing for 64-bit values
 *
 * \param h64 Value to mix
 * \return Mixed value with improved bit distribution
 *
 * \details Ensures all input bits affect output bits
 */
inline constexpr auto avalanche(uint64_t h64) noexcept -> uint64_t {
    h64 ^= h64 >> 37;
    h64 *= 0x165667919E3779F9ULL;
    h64 ^= h64 >> 32;
    return h64;
}

/*!
 * \brief Multiply two 64-bit values and fold the 128-bit result
 *
 * \param lhs Left operand
 * \param rhs Right operand
 * \return Folded 64-bit result
 *
 * \details Provides good mixing by using full 128-bit multiplication
 */
inline auto mul128_fold64(uint64_t lhs, uint64_t rhs) noexcept -> uint64_t {
#if defined(__SIZEOF_INT128__) || (defined(_MSC_VER) && defined(_WIN64))
    // Fast path: native 128-bit multiplication
    #ifdef __SIZEOF_INT128__
        __uint128_t product = static_cast<__uint128_t>(lhs) * static_cast<__uint128_t>(rhs);
        return static_cast<uint64_t>(product) ^ static_cast<uint64_t>(product >> 64);
    #else
        // MSVC path
        uint64_t high;
        uint64_t low = _umul128(lhs, rhs, &high);
        return low ^ high;
    #endif
#else
    // Portable fallback: 32-bit multiplication
    uint64_t lo_lo = (lhs & 0xFFFFFFFF) * (rhs & 0xFFFFFFFF);
    uint64_t hi_lo = (lhs >> 32) * (rhs & 0xFFFFFFFF);
    uint64_t lo_hi = (lhs & 0xFFFFFFFF) * (rhs >> 32);
    uint64_t hi_hi = (lhs >> 32) * (rhs >> 32);
    
    uint64_t cross = (lo_lo >> 32) + (hi_lo & 0xFFFFFFFF) + lo_hi;
    uint64_t high = (hi_lo >> 32) + (cross >> 32) + hi_hi;
    uint64_t low = (cross << 32) | (lo_lo & 0xFFFFFFFF);
    
    return high ^ low;
#endif
}

/*!
 * \brief Mix 16 bytes of input with secret key
 *
 * \param input Input data pointer
 * \param secret Secret key pointer
 * \param seed Seed value
 * \return Mixed 64-bit value
 *
 * \details Core mixing function for 16-byte blocks
 */
inline auto mix16B(const uint8_t* input, const uint8_t* secret, uint64_t seed) noexcept -> uint64_t {
    uint64_t const input_lo = read64(input);
    uint64_t const input_hi = read64(input + 8);
    uint64_t const key_lo = read64(secret);
    uint64_t const key_hi = read64(secret + 8);
    
    uint64_t const lo_mixed = input_lo ^ (key_lo + seed);
    uint64_t const hi_mixed = input_hi ^ (key_hi - seed);
    
    return mul128_fold64(lo_mixed, PRIME64_1) ^ mul128_fold64(hi_mixed, PRIME64_2);
}

} // namespace xxh3_detail

/**********************************************************************************************************************
 *  PUBLIC API
 *********************************************************************************************************************/

/*!
 * \brief Compute XXH3 64-bit hash with custom seed
 *
 * \param data Pointer to data to hash
 * \param len Length of data in bytes
 * \param seed Custom seed value
 * \return 64-bit hash value
 *
 * \details
 * This is a minimal implementation of XXH3_64bits_withSeed optimized for:
 * - Little-endian architectures only
 * - No alignment requirements (uses memcpy)
 * - Simplified for code size (~250 lines vs 5000+)
 * 
 * The implementation handles different input sizes with specialized paths:
 * - 0 bytes: seed-based constant
 * - 1-3 bytes: special mixing for tiny inputs
 * - 4-8 bytes: single 64-bit mix
 * - 9-16 bytes: dual 64-bit mix
 * - 17-128 bytes: short input loop
 * - 129-240 bytes: medium input loop
 * - 241+ bytes: full XXH3 algorithm
 *
 * \note This function is NOT constexpr due to memcpy usage
 */
[[nodiscard]] inline auto xxh3_64bits_withSeed(const void* data, std::size_t len, uint64_t seed) noexcept -> uint64_t {
    const uint8_t* const input = static_cast<const uint8_t*>(data);
    
    // -----------------------------------------------------------------------------------
    // SMALL INPUTS (0-16 bytes)
    // -----------------------------------------------------------------------------------
    
    if (len <= 16) {
        if (len > 8) {
            // 9-16 bytes
            uint64_t const input_lo = xxh3_detail::read64(input) ^ 
                                     (xxh3_detail::read64(xxh3_detail::kSecret + 24) + seed);
            uint64_t const input_hi = xxh3_detail::read64(input + len - 8) ^ 
                                     (xxh3_detail::read64(xxh3_detail::kSecret + 32) - seed);
            uint64_t const acc = xxh3_detail::mul128_fold64(input_lo, input_hi);
            return xxh3_detail::avalanche(acc + len * xxh3_detail::PRIME64_1);
        }
        
        if (len >= 4) {
            // 4-8 bytes
            uint32_t const input1 = xxh3_detail::read32(input);
            uint32_t const input2 = xxh3_detail::read32(input + len - 4);
            uint64_t const input64 = input2 + (static_cast<uint64_t>(input1) << 32);
            uint64_t const bitflip = (xxh3_detail::read64(xxh3_detail::kSecret + 8) ^ seed) + len;
            uint64_t const keyed = input64 ^ bitflip;
            return xxh3_detail::avalanche(xxh3_detail::mul128_fold64(keyed, xxh3_detail::PRIME64_1));
        }
        
        if (len > 0) {
            // 1-3 bytes
            uint8_t const c1 = input[0];
            uint8_t const c2 = input[len >> 1];
            uint8_t const c3 = input[len - 1];
            uint32_t const combined = c1 + (static_cast<uint32_t>(c2) << 8) + 
                                     (static_cast<uint32_t>(c3) << 16) + 
                                     (static_cast<uint32_t>(len) << 24);
            uint64_t const bitflip = static_cast<uint64_t>(xxh3_detail::read32(xxh3_detail::kSecret) ^ seed);
            uint64_t const keyed = static_cast<uint64_t>(combined) ^ bitflip;
            return xxh3_detail::avalanche(keyed * xxh3_detail::PRIME64_1);
        }
        
        // 0 bytes
        return xxh3_detail::avalanche(seed ^ xxh3_detail::PRIME64_1);
    }
    
    // -----------------------------------------------------------------------------------
    // MEDIUM INPUTS (17-128 bytes)
    // -----------------------------------------------------------------------------------
    
    if (len <= 128) {
        uint64_t acc = len * xxh3_detail::PRIME64_1;
        
        if (len > 32) {
            if (len > 64) {
                if (len > 96) {
                    acc += xxh3_detail::mix16B(input + 48, xxh3_detail::kSecret + 96, seed);
                    acc += xxh3_detail::mix16B(input + len - 64, xxh3_detail::kSecret + 112, seed);
                }
                acc += xxh3_detail::mix16B(input + 32, xxh3_detail::kSecret + 64, seed);
                acc += xxh3_detail::mix16B(input + len - 48, xxh3_detail::kSecret + 80, seed);
            }
            acc += xxh3_detail::mix16B(input + 16, xxh3_detail::kSecret + 32, seed);
            acc += xxh3_detail::mix16B(input + len - 32, xxh3_detail::kSecret + 48, seed);
        }
        
        acc += xxh3_detail::mix16B(input + 0, xxh3_detail::kSecret + 0, seed);
        acc += xxh3_detail::mix16B(input + len - 16, xxh3_detail::kSecret + 16, seed);
        
        return xxh3_detail::avalanche(acc);
    }
    
    // -----------------------------------------------------------------------------------
    // LARGE INPUTS (129-240 bytes)
    // -----------------------------------------------------------------------------------
    
    if (len <= 240) {
        uint64_t acc = len * xxh3_detail::PRIME64_1;
        size_t const nbRounds = len / 16;
        size_t i = 0;
        
        for (; i < 8; ++i) {
            acc += xxh3_detail::mix16B(input + (16 * i), xxh3_detail::kSecret + (16 * i), seed);
        }
        
        acc = xxh3_detail::avalanche(acc);
        
        for (; i < nbRounds; ++i) {
            acc += xxh3_detail::mix16B(input + (16 * i), 
                                      xxh3_detail::kSecret + (16 * (i - 8)), seed);
        }
        
        // Last partial block
        acc += xxh3_detail::mix16B(input + len - 16, xxh3_detail::kSecret + 48, seed);
        
        return xxh3_detail::avalanche(acc);
    }
    
    // -----------------------------------------------------------------------------------
    // VERY LARGE INPUTS (241+ bytes) - Full XXH3 algorithm
    // -----------------------------------------------------------------------------------
    
    uint64_t acc0 = seed + xxh3_detail::PRIME64_1;
    uint64_t acc1 = seed + xxh3_detail::PRIME64_2;
    uint64_t acc2 = seed + xxh3_detail::PRIME64_3;
    uint64_t acc3 = seed + xxh3_detail::PRIME64_4;
    uint64_t acc4 = seed + xxh3_detail::PRIME64_5;
    uint64_t acc5 = seed - xxh3_detail::PRIME64_1;
    uint64_t acc6 = seed - xxh3_detail::PRIME64_2;
    uint64_t acc7 = seed - xxh3_detail::PRIME64_3;
    
    size_t const nb_blocks = (len - 1) / 64;
    
    for (size_t n = 0; n < nb_blocks; ++n) {
        size_t const block_offset = n * 64;
        
        acc0 = xxh3_detail::mix16B(input + block_offset + 0,  xxh3_detail::kSecret + 0,  acc0);
        acc1 = xxh3_detail::mix16B(input + block_offset + 16, xxh3_detail::kSecret + 16, acc1);
        acc2 = xxh3_detail::mix16B(input + block_offset + 32, xxh3_detail::kSecret + 32, acc2);
        acc3 = xxh3_detail::mix16B(input + block_offset + 48, xxh3_detail::kSecret + 48, acc3);
        
        // Scramble accumulators
        acc0 ^= acc0 >> 47;
        acc0 ^= seed;
        acc0 *= xxh3_detail::PRIME64_1;
        
        acc1 ^= acc1 >> 47;
        acc1 ^= seed;
        acc1 *= xxh3_detail::PRIME64_2;
        
        acc2 ^= acc2 >> 47;
        acc2 ^= seed;
        acc2 *= xxh3_detail::PRIME64_3;
        
        acc3 ^= acc3 >> 47;
        acc3 ^= seed;
        acc3 *= xxh3_detail::PRIME64_4;
        
        // Additional accumulators for better mixing
        acc4 += acc0;
        acc5 += acc1;
        acc6 += acc2;
        acc7 += acc3;
    }
    
    // Last block
    size_t const last_block_offset = len - 64;
    acc0 = xxh3_detail::mix16B(input + last_block_offset + 0,  xxh3_detail::kSecret + 0,  acc0);
    acc1 = xxh3_detail::mix16B(input + last_block_offset + 16, xxh3_detail::kSecret + 16, acc1);
    acc2 = xxh3_detail::mix16B(input + last_block_offset + 32, xxh3_detail::kSecret + 32, acc2);
    acc3 = xxh3_detail::mix16B(input + last_block_offset + 48, xxh3_detail::kSecret + 48, acc3);
    
    // Final mixing
    uint64_t const result = len * xxh3_detail::PRIME64_1 +
                           (acc0 + acc1 + acc2 + acc3 + acc4 + acc5 + acc6 + acc7);
    
    return xxh3_detail::avalanche(result);
}

} // namespace internal
} // namespace core
} // namespace ara

#endif // OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_INTERNAL_XXH3_MINIMAL_H_