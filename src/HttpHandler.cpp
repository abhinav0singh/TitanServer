#include "HttpHandler.h"
#include "FileLoader.h"
#include "LRUCache.h"
#include "Logger.h"

// To reach project-root/www we must go up THREE levels.
static const std::string WEB_ROOT = "../../../www";

// Cache: path -> file contents
static LRUCache<std::string, std::string> fileCache(5);

HttpResponse HttpHandler::handle(const HttpRequest& request) {

    // Only GET is supported for now
    if (request.method != HttpMethod::GET) {
        HttpResponse res(500);
        res.setHeader("Content-Type", "text/plain");
        res.setBody("Unsupported HTTP method");
        return res;
    }

    // Map "/" → "/index.html"
    std::string path = request.path;
    if (path == "/") {
        path = "/index.html";
    }

    std::string filePath = WEB_ROOT + path;

    // Log the resolved path (debugging + visibility)
    Logger::instance().info("Trying to load file: " + filePath);

    std::string content;

    // 1 Try cache first
    if (fileCache.get(filePath, content)) {
        Logger::instance().info("Cache hit: " + filePath);
    }
    // 2️ Cache miss → load from disk
    else {
        Logger::instance().info("Cache miss: " + filePath);

        auto loaded = FileLoader::loadFile(filePath);
        if (!loaded) {
            HttpResponse res(404);
            res.setHeader("Content-Type", "text/plain");
            res.setBody("404 Not Found");
            return res;
        }

        content = *loaded;
        fileCache.put(filePath, content);
    }

    // 3️ Build HTTP response
    HttpResponse res(200);
    res.setHeader(
        "Content-Type",
        FileLoader::contentTypeForPath(filePath)
    );
    res.setBody(content);

    return res;
}
