#include "HttpHandler.h"
#include "FileLoader.h"
#include "LRUCache.h"
#include "Logger.h"

#include <atomic>
#include <thread>
#include <chrono>


// We need to go up THREE levels to reach project root
static const std::string WEB_ROOT = "C:/Users/abhin/Desktop/TitanServer/TitanServer/www";

// Cache: file path -> file contents
static LRUCache<std::string, std::string> fileCache(5);

// Global request counter (thread-safe)
static std::atomic<int> requestCount{ 0 };

HttpResponse HttpHandler::handle(const HttpRequest& request) {
    requestCount++;

    
    if (request.method != HttpMethod::GET) {
        HttpResponse res(500);
        res.setHeader("Content-Type", "text/plain");
        res.setBody("Unsupported HTTP method");
        return res;
    }

    
    if (request.path == "/health") {
        HttpResponse res(200);
        res.setHeader("Content-Type", "application/json");
        res.setBody("{\"status\":\"ok\"}");
        return res;
    }

    if (request.path == "/stats") {
        HttpResponse res(200);
        res.setHeader("Content-Type", "application/json");

        res.setBody(
            "{ \"requests\": " +
            std::to_string(requestCount.load()) +
            " }"
        );
        return res;
    }

   
    if (request.path == "/slow") {
        Logger::instance().info("Simulating slow request...");
        std::this_thread::sleep_for(std::chrono::seconds(3));

        HttpResponse res(200);
        res.setHeader("Content-Type", "text/plain");
        res.setBody("Slow response finished");
        return res;
    }

    // DASHBOARD UI
    if (request.path == "/dashboard") {
        std::string dashboardPath = WEB_ROOT + "/dashboard.html";

        auto page = FileLoader::loadFile(dashboardPath);
        if (!page) {
            HttpResponse res(500);
            res.setBody("Dashboard not found");
            return res;
        }

        HttpResponse res(200);
        res.setHeader("Content-Type", "text/html");
        res.setBody(*page);
        return res;
    }

   

       // Map "/" → "/index.html"
    std::string path = request.path;
    if (path == "/") {
        path = "/index.html";
    }

    std::string filePath = WEB_ROOT + path;
    Logger::instance().debug("Trying to load file: " + filePath);

    std::string content;

    // 1️ Cache lookup
    if (fileCache.get(filePath, content)) {
        Logger::instance().debug("Cache hit: " + filePath);
    }
    // 2️ Load from disk
    else {
        Logger::instance().debug("Cache miss: " + filePath);

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

    // 3️ Build response
    HttpResponse res(200);
    res.setHeader(
        "Content-Type",
        FileLoader::contentTypeForPath(filePath)
    );
    res.setBody(content);

    return res;
}
