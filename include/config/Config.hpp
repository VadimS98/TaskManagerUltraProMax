#pragma once

#include <string>
#include <vector>
#include <cstdint>

enum class NetworkMode {
    Mock,
    Real
};


enum class LaunchMode {
    Admin,
    User
};

class Config {
public:
    // Singleton
    static Config& Instance();

    bool Load(const std::string& configPath);

    // Network
    NetworkMode GetNetworkMode() const { return m_networkMode; }
    std::string GetServerUrl() const { return m_serverUrl; }
    std::string GetLogFilePath() const { return m_logFilePath; }

    // Crypto
    std::vector<std::uint8_t> GetCryptoKey() const { return m_cryptoKey; }
    std::vector<std::uint8_t> GetCryptoIV() const { return m_cryptoIV; }

    // UI
    std::size_t GetMaxModulesToShow() const { return m_maxModulesToShow; }
    int GetRefreshIntervalMs() const { return m_refreshIntervalMs; }

    // Launch
    LaunchMode GetLaunchMode() const { return m_launchMode; }
    bool GetHideInaccessible() const { return m_hideInaccessible; }

private:
    Config();
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    static std::vector<std::uint8_t> HexToBytes(const std::string& hex);

    // Network
    NetworkMode m_networkMode;
    std::string m_serverUrl;
    std::string m_logFilePath;

    // Crypto
    std::vector<std::uint8_t> m_cryptoKey;
    std::vector<std::uint8_t> m_cryptoIV;

    // UI
    std::size_t m_maxModulesToShow;
    int m_refreshIntervalMs;

    // Launch Settings
    LaunchMode m_launchMode;
    bool m_hideInaccessible;

    bool m_isLoaded;
};
