#include <fstream>
#include <chrono>
#include <ctime>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "network/NetworkClient.hpp"

#include "crypto/CryptoEngine.hpp"
#include "config/Config.hpp"


using json = nlohmann::json;

NetworkClient::NetworkClient() {
    m_serverUrl = Config::Instance().GetServerUrl();
    m_logFilePath = Config::Instance().GetLogFilePath();
}

bool NetworkClient::SendData(const std::string& requestJson, std::string& outError) {
    if (Config::Instance().GetNetworkMode() == NetworkMode::Mock)
        return SaveToLogFile(requestJson, outError);

    else
        return SendToServer(requestJson, outError);
}

std::optional<std::string> NetworkClient::GetData(const std::string& requestJson, std::string& outError) {
    if (Config::Instance().GetNetworkMode() == NetworkMode::Mock) {
        SaveGetToLogFile(requestJson, "MOCK", outError);

        auto key = Config::Instance().GetCryptoKey();
        auto iv = Config::Instance().GetCryptoIV();

        if (key.empty() || iv.empty()) {
            outError = "Mock mode: Crypto key or IV not configured";
            return std::nullopt;
        }

        try {
            std::string testData = "Mock test data: PID:1234:notepad.exe,PID:5678:chrome.exe";
            std::vector<std::uint8_t> plainBytes(testData.begin(), testData.end());

            CryptoEngine crypto(key, iv);
            std::vector<std::uint8_t> encryptedBytes = crypto.Encrypt(plainBytes);

            std::string encryptedHex;
            encryptedHex.reserve(encryptedBytes.size() * 2);
            for (std::uint8_t byte : encryptedBytes) {
                char buf[3];
                sprintf_s(buf, sizeof(buf), "%02X", byte);
                encryptedHex += buf;
            }

            return encryptedHex;
        }

        catch (const std::exception& e)
        {
            outError = std::string("Mock mode encryption failed: ") + e.what();
            return std::nullopt;
        }
    }

    SaveGetToLogFile(requestJson, "REAL", outError);
    return GetFromServer(requestJson, outError);
}

bool NetworkClient::SendToServer(const std::string& requestJson, std::string& outError) {
    try {
        std::string host = "172.245.127.93";
        std::string path = "/p/applicants.php";

        httplib::Client client("http://" + host);
        client.set_connection_timeout(5, 0);
        client.set_read_timeout(10, 0);

        auto response = client.Post(path, requestJson, "application/json");

        if (!response) {
            outError = "Network error: no response from server (timeout or connection refused)";
            return false;
        }

        if (response->status != 200) {
            outError = "HTTP error: " + std::to_string(response->status) + " " + response->reason;
            return false;
        }

        return true;
    }

    catch (const std::exception& e) {
        outError = std::string("Exception: ") + e.what();
        return false;
    }
}

std::optional<std::string> NetworkClient::GetFromServer(const std::string& requestJson, std::string& outError) {
    try {
        std::string host = "172.245.127.93";
        std::string path = "/p/applicants.php";

        httplib::Client client("http://" + host);
        client.set_connection_timeout(5, 0);
        client.set_read_timeout(10, 0);

        auto response = client.Post(path, requestJson, "application/json");

        if (!response) {
            outError = "Network error: no response from server";
            SaveGetResponseToLogFile("", 0, outError, outError);
            return std::nullopt;
        }

        if (response->status != 200) {
            outError = "HTTP error: " + std::to_string(response->status);
            SaveGetResponseToLogFile(response->body, response->status, outError, outError);
            return std::nullopt;
        }

        SaveGetResponseToLogFile(response->body, response->status, "", outError);

        json responseJson = json::parse(response->body);

        if (!responseJson.contains("data")) {
            outError = "Response missing 'data' field";
            SaveGetResponseToLogFile("Missing 'data' field. Full JSON: " + response->body, 200, outError, outError);
            return std::nullopt;
        }

        std::string encryptedData = responseJson["data"].get<std::string>();

        if (encryptedData.empty()) {
            outError = "Received empty 'data' field";
            SaveGetResponseToLogFile("Empty 'data' field", 200, outError, outError);
            return std::nullopt;
        }

        return encryptedData;
    }

    catch (const json::parse_error& e) {
        outError = std::string("JSON parse error: ") + e.what();
        SaveGetResponseToLogFile("", 0, outError, outError);
        return std::nullopt;
    }

    catch (const std::exception& e) {
        outError = std::string("Exception: ") + e.what();
        SaveGetResponseToLogFile("", 0, outError, outError);
        return std::nullopt;
    }
}

bool NetworkClient::SaveGetToLogFile(const std::string& requestJson, const std::string& mode, std::string& outError) {
    try {
        std::ofstream file(m_logFilePath, std::ios::app);
        if (!file.is_open()) {
            outError = "Failed to open log file: " + m_logFilePath;
            return false;
        }

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        std::tm tm{};
        localtime_s(&tm, &time);

        std::array<char, 100> buffer{};
        std::strftime(buffer.data(), buffer.size(), "%Y-%m-%d %H:%M:%S", &tm);

        file << "=== GET Request at " << buffer.data() << " [" << mode << " MODE] ===" << "\n";
        file << requestJson << "\n\n";
        file.close();

        return true;
    }
    catch (const std::exception& e) {
        outError = std::string("Exception while writing to log: ") + e.what();
        return false;
    }
}

bool NetworkClient::SaveGetResponseToLogFile(const std::string& response, int httpStatus, const std::string& errorMsg, std::string& outError) {
    try {
        std::ofstream file(m_logFilePath, std::ios::app);
        if (!file.is_open()) {
            outError = "Failed to open log file: " + m_logFilePath;
            return false;
        }

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        std::tm tm{};
        localtime_s(&tm, &time);

        std::array<char, 100> buffer{};
        std::strftime(buffer.data(), buffer.size(), "%Y-%m-%d %H:%M:%S", &tm);

        file << "=== GET Response at " << buffer.data() << " ===" << "\n";

        if (httpStatus > 0) {
            file << "HTTP Status: " << httpStatus << "\n";
        }

        if (!errorMsg.empty()) {
            file << "ERROR: " << errorMsg << "\n";
        }

        file << "Response length: " << response.length() << "\n";
        file << "Response body:\n" << response << "\n\n";
        file.close();

        return true;
    }
    catch (const std::exception& e) {
        outError = std::string("Exception while writing to log: ") + e.what();
        return false;
    }
}

bool NetworkClient::SaveToLogFile(const std::string& requestJson, std::string& outError) {
    try {
        std::ofstream file(m_logFilePath, std::ios::app);
        if (!file.is_open()) {
            outError = "Failed to open log file: " + m_logFilePath;
            return false;
        }

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        std::tm tm{};
        localtime_s(&tm, &time);

        std::array<char, 100> buffer{};
        std::strftime(buffer.data(), buffer.size(), "%Y-%m-%d %H:%M:%S", &tm);

        std::string timeStr(buffer.data());

        file << "=== POST Request at " << timeStr << " ===" << "\n";
        file << requestJson << "\n\n";
        file.close();

        return true;
    }

    catch (const std::exception& e) {
        outError = std::string("Exception while writing to log: ") + e.what();
        return false;
    }
}
