#include <nlohmann/json.hpp>

#include "network/RequestBuilder.hpp"

using json = nlohmann::json;

std::string RequestBuilder::BuildSendRequest(const std::string& rid, const std::string& encryptedDataHex) {
    json request;
    request["cmd"] = 1;
    request["rid"] = rid;
    request["data"] = encryptedDataHex;

    return request.dump();
}

std::string RequestBuilder::BuildGetRequest(const std::string& rid) {
    json request;
    request["cmd"] = 2;
    request["rid"] = rid;

    return request.dump();
}