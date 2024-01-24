#include "cipher.h"
#include "misc.h"

#define BOOST_TEST_MODULE Cipher Tests
#include <boost/test/included/unit_test.hpp>

using CredentialType = std::array<char, 32>;

uint32_t test_and_get_initial_key()
{
    const auto username = init_credential<CredentialType>("testuser");
    const auto password = init_credential<CredentialType>("testpass");

    const auto initial_key = get_initial_key(87, username, password);

    BOOST_TEST(initial_key == 0x577F77);

    return initial_key;
}

BOOST_AUTO_TEST_CASE(cipher_key_example_test)
{
    using CipherKeyType = std::array<std::uint8_t, 80>;

    const auto initial_key = test_and_get_initial_key();

    CipherKeyType cipher_key;
    std::generate(cipher_key.begin(), cipher_key.end(), [key = initial_key]() mutable
                  { key = next_key(key); return key % 256; });

    const CipherKeyType expected_cipher_key = {
        0xE5, 0xBA, 0x6B, 0xC9, 0xCE, 0xEF, 0xFC, 0x86,
        0x48, 0xE1, 0x06, 0xC8, 0x62, 0xF3, 0xB1, 0x96,
        0x18, 0x72, 0xC4, 0xAD, 0xE2, 0x74, 0x9D, 0x13,
        0x51, 0xB7, 0x24, 0x8E, 0xB0, 0x2A, 0x1B, 0xB9,
        0xFE, 0x60, 0x19, 0xDF, 0x2D, 0x62, 0xF4, 0x1E,
        0xFF, 0xCC, 0x16, 0x98, 0xF2, 0x44, 0x2E, 0xCF,
        0x5D, 0xD2, 0xA4, 0x0E, 0x30, 0xA9, 0x2F, 0x3D,
        0x32, 0x83, 0x01, 0xA6, 0xE7, 0x95, 0xAB, 0x09,
        0x0E, 0x30, 0xA9, 0x2F, 0x3D, 0x32, 0x83, 0x00,
        0x3A, 0xEC, 0xB6, 0xB8, 0x91, 0xF7, 0x65, 0x3A};

    BOOST_TEST(cipher_key == expected_cipher_key);
}

BOOST_AUTO_TEST_CASE(cipher_test)
{
    using BufferType = std::array<std::uint8_t, 4>;

    const auto initial_key = test_and_get_initial_key();

    const BufferType plain_text = {'t', 'e', 's', 't'};

    auto text = plain_text;
    cipher(text, initial_key);

    const BufferType expected_encrypted_text = {0x91, 0xdf, 0x18, 0xbd};
    BOOST_TEST(text == expected_encrypted_text);

    cipher(text, initial_key);

    BOOST_TEST(text == plain_text);
}
