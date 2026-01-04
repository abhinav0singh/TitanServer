#include "HttpParser.h"
#include <sstream>

HttpMethod HttpParser::parseMethod(const std::string& method) {
    if (method == "GET") return HttpMethod::GET;
    if (method == "POST") return HttpMethod::POST;
    return HttpMethod::UNKNOWN;
}

HttpRequest HttpParser::parse(const std::string& rawRequest) {
    HttpRequest request;
    std::istringstream stream(rawRequest);
    std::string line;

    // Request line
    std::getline(stream, line);
    std::istringstream requestLine(line);

    std::string methodStr;
    requestLine >> methodStr >> request.path >> request.version;
    request.method = parseMethod(methodStr);

    // Headers
    while (std::getline(stream, line) && line != "\r") {
        auto colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            if (!value.empty() && value[0] == ' ')
                value.erase(0, 1);
            if (!value.empty() && value.back() == '\r')
                value.pop_back();

            request.headers[key] = value;
        }
    }

    // Body (simple version)
    std::string body;
    while (std::getline(stream, line)) {
        body += line;
    }
    request.body = body;

    return request;
}
