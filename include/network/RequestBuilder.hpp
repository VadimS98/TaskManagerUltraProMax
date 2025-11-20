#pragma once

#include <string>

class RequestBuilder {
public:
    static std::string BuildSendRequest(const std::string& rid, const std::string& encryptedDataHex);
    static std::string BuildGetRequest(const std::string& rid);
};