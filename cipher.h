#pragma once

#include <cstdint>
#include <span>
#include <numeric>
#include <algorithm>
#include <cassert>

template <typename T>
std::uint8_t sum_buffer(std::span<const T> buffer)
{
    return std::accumulate(buffer.begin(), buffer.end(), 0);
}

// UNUSED: 'Decryption of Echo Request Message' indicates that computing
// a sum complement checksum is a required step, but then initial_key is
// computed with the sum of the buffer elements (variable names suggest
// so, username_sum and password_sum, which is confirmed by the example)
// template <typename T>
// std::uint8_t checksum(std::span<const T> buffer)
// {
//     const auto sum = sum_buffer(buffer);
//     return ~sum + 1;
// }

inline std::uint32_t get_initial_key(const uint8_t message_sequence, std::span<const char> username, std::span<const char> password)
{
    const auto username_sum = sum_buffer(username);
    const auto password_sum = sum_buffer(password);
    return (message_sequence << 16) | (username_sum << 8) | password_sum;
}

inline std::uint32_t next_key(std::uint32_t key)
{
    return (key * 1103515245 + 12345) % 0x7FFFFFFF;
}

inline void cipher(std::span<std::uint8_t> text, const uint32_t initial_key)
{
    std::transform(text.begin(), text.end(), text.begin(),
                   [key = initial_key](auto c) mutable
                   { key = next_key(key); return c ^ (key % 256); });
}

inline void cipher_helper(std::span<std::uint8_t> buffer, const uint8_t message_sequence, std::span<const char> username, std::span<const char> password)
{
    const auto initial_key = get_initial_key(message_sequence, username, password);

    cipher(buffer, initial_key);
}
