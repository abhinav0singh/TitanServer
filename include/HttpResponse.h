#pragma once

#include <string>
#include <unordered_map>

class HttpResponse {
public:
    HttpResponse(int statusCode = 200);

    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);

    std::string toString() const;

private:
    int statusCode_;
    std::string statusMessage_;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;

    static std::string statusMessageForCode(int code);
};
