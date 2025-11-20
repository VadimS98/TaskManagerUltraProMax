#include <stdexcept>

#include <cryptopp/aes.h>
#include <cryptopp/modes.h>      // CBC_Mode
#include <cryptopp/filters.h>    // StreamTransformationFilter, ArraySource, ArraySink
#include <cryptopp/secblock.h>

#include "crypto/CryptoEngine.hpp"

using std::uint8_t;

CryptoEngine::CryptoEngine(const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv)
    : m_key(key) , m_iv(iv) {
    if (m_key.size() != CryptoPP::AES::MAX_KEYLENGTH) // 32 байта
        throw std::runtime_error("CryptoEngine: key must be 32 bytes (AES-256)");

    if (m_iv.size() != CryptoPP::AES::BLOCKSIZE) // 16 байт
        throw std::runtime_error("CryptoEngine: IV must be 16 bytes (AES block size)");
}

std::vector<uint8_t> CryptoEngine::Encrypt(const std::vector<uint8_t>& plainData) const {
    if (plainData.empty())
        return {};

    CryptoPP::SecByteBlock keyBlock(m_key.data(), m_key.size());
    CryptoPP::SecByteBlock ivBlock(m_iv.data(), m_iv.size());

    CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption enc;
    enc.SetKeyWithIV(keyBlock, keyBlock.size(), ivBlock, ivBlock.size());

    std::vector<uint8_t> cipher;

    CryptoPP::StringSource ss(
        plainData.data(),
        plainData.size(),
        true,
        new CryptoPP::StreamTransformationFilter(
            enc,
            new CryptoPP::VectorSink(cipher)
        )
    );

    return cipher;
}

std::vector<uint8_t> CryptoEngine::Decrypt(const std::vector<uint8_t>& cipherData) const {
    if (cipherData.empty())
        return {};

    CryptoPP::SecByteBlock keyBlock(m_key.data(), m_key.size());
    CryptoPP::SecByteBlock ivBlock(m_iv.data(), m_iv.size());

    CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption decryptor;
    decryptor.SetKeyWithIV(keyBlock, keyBlock.size(), ivBlock, ivBlock.size());

    std::vector<uint8_t> plain;

    CryptoPP::StringSource ss(
        cipherData.data(),
        cipherData.size(),
        true,
        new CryptoPP::StreamTransformationFilter(
            decryptor,
            new CryptoPP::VectorSink(plain)
        )
    );

    return plain;
}