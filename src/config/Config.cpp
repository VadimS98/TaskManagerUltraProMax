#include <fstream>
#include <sstream>
#include <iomanip>

#include <windows.h> 

#include <nlohmann/json.hpp>

#include "config/Config.hpp"

using json = nlohmann::json;

Config::Config()
    : m_networkMode(NetworkMode::Mock)
    , m_serverUrl("")
    , m_logFilePath("network_log.txt")
    , m_maxModulesToShow(20)
    , m_refreshIntervalMs(1000)
    , m_launchMode(LaunchMode::User)
    , m_hideInaccessible(true)
    , m_isLoaded(false)
{}

Config& Config::Instance() {
    static Config instance;
    return instance;
}

bool Config::Load(const std::string& configPath) {
    try {
        std::ifstream file(configPath);
        if (!file.is_open())
            return false;

        json config;
        try {
            config = json::parse(file);
        }
        catch (const json::parse_error& e) {
            std::string error = "JSON parse error: ";
            error += e.what();
            MessageBoxA(nullptr, error.c_str(), "Config Error", MB_OK);
            return false;
        }

        // === Network settings ===
        if (config.contains("network")) {
            const auto& network = config["network"];

            // Mode
            if (network.contains("mode"))  {
                std::string mode = network["mode"];
                m_networkMode = (mode == "real") ? NetworkMode::Real : NetworkMode::Mock;
            }

            // Server URL
            if (network.contains("server_url")) {
                m_serverUrl = network["server_url"];
            }

            // Log file path
            if (network.contains("log_file")) {
                m_logFilePath = network["log_file"];
            }
        }

        // === Crypto settings ===
        if (config.contains("crypto")) {
            const auto& crypto = config["crypto"];

            // Key (hex string -> bytes)
            if (crypto.contains("key_hex")) {
                std::string keyHex = crypto["key_hex"];
                m_cryptoKey = HexToBytes(keyHex);
            }

            // IV (hex string -> bytes)
            if (crypto.contains("iv_hex")) {
                std::string ivHex = crypto["iv_hex"];
                m_cryptoIV = HexToBytes(ivHex);
            }
        }

        // === UI settings ===
        if (config.contains("ui")) {
            const auto& ui = config["ui"];

            // Max modules to show
            if (ui.contains("max_modules_to_show")) {
                m_maxModulesToShow = ui["max_modules_to_show"];
            }

            // Refresh interval
            if (ui.contains("refresh_interval_ms")) {
                m_refreshIntervalMs = ui["refresh_interval_ms"];
            }
        }

        if (config.contains("settings")) {
            const auto& settings = config["settings"];

            // Desired launch mode
            if (settings.contains("launch")) {
                std::string mode = settings["launch"];
                m_launchMode = (mode == "admin") ? LaunchMode::Admin : LaunchMode::User;
            }

            // What to do with inaccessible
            if (settings.contains("hide_inaccessible")) {
                m_hideInaccessible = settings["hide_inaccessible"];
            }
        }

        m_isLoaded = true;
        return true;
    }

    catch (const json::parse_error&) {
        return false;
    }

    catch (const std::exception&) {
        return false;
    }
}

std::vector<std::uint8_t> Config::HexToBytes(const std::string& hex) {
    std::vector<std::uint8_t> bytes;

    // Hex строка => чётное количество символов
    if (hex.length() % 2 != 0)
        return bytes;

    bytes.reserve(hex.length() / 2);

    for (std::size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        std::uint8_t byte = static_cast<std::uint8_t>(std::stoi(byteString, nullptr, 16));
        bytes.push_back(byte);
    }

    return bytes;
}
