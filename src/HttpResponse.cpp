#include "HttpResponse.h"
#include <sstream>

HttpResponse::HttpResponse(int statusCode)
    : statusCode_(statusCode),
    statusMessage_(statusMessageForCode(statusCode)) {
}

void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
}

void HttpResponse::setBody(const std::string& body) {
    body_ = body;
    headers_["Content-Length"] = std::to_string(body.size());
}

std::string HttpResponse::toString() const {
    std::ostringstream response;

    response << "HTTP/1.1 "
        << statusCode_ << " "
        << statusMessage_ << "\r\n";

    for (const auto& [k, v] : headers_) {
        response << k << ": " << v << "\r\n";
    }

    response << "\r\n";
    response << body_;

    return response.str();
}

std::string HttpResponse::statusMessageForCode(int code) {
    switch (code) {
    case 200: return "OK";
    case 404: return "Not Found";
    case 500: return "Internal Server Error";
    default:  return "Unknown";
    }
}
