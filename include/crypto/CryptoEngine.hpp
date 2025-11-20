#pragma once

#include <vector>
#include <cstdint>

//(AES-256-CBC).
class CryptoEngine {
public:
    CryptoEngine(const std::vector<std::uint8_t>& key,
        const std::vector<std::uint8_t>& iv);

    std::vector<std::uint8_t> Encrypt(const std::vector<std::uint8_t>& plainData) const;
    std::vector<std::uint8_t> Decrypt(const std::vector<std::uint8_t>& cipherData) const;

private:
    std::vector<std::uint8_t> m_key;  // 32
    std::vector<std::uint8_t> m_iv;   // 16
};
