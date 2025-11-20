#pragma once

#include <string>
#include <optional>

class NetworkClient {
public:
    NetworkClient();

    bool SendData(const std::string& requestJson, std::string& outError);
    std::optional<std::string> GetData(const std::string& requestJson, std::string& outError);

private:
    bool SendToServer(const std::string& requestJson, std::string& outError);
    std::optional<std::string> GetFromServer(const std::string& requestJson, std::string& outError);

    bool SaveToLogFile(const std::string& requestJson, std::string& outError);
    bool SaveGetToLogFile(const std::string& requestJson, const std::string& mode, std::string& outError);
    bool SaveGetResponseToLogFile(const std::string& response, int httpStatus, const std::string& errorMsg, std::string& outError);

    std::string m_serverUrl;
    std::string m_logFilePath;
};
