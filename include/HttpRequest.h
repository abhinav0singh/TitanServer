#pragma once

#include <string>
#include <unordered_map>

enum class HttpMethod {
    GET,
    POST,
    UNKNOWN
};

struct HttpRequest {
    HttpMethod method;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};
