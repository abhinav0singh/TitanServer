#pragma once

#include <string>
#include <optional>

class FileLoader {
public:
    static std::optional<std::string> loadFile(const std::string& path);
    static std::string contentTypeForPath(const std::string& path);
};
