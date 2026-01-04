#include "Logger.h"
#include "ThreadPool.h"
#include "LRUCache.h"

#include <string>

int main() {
    Logger::instance().info("TitanServer starting...");

    LRUCache<std::string, std::string> cache(2);

    cache.put("a", "alpha");
    cache.put("b", "beta");

    std::string value;
    if (cache.get("a", value)) {
        Logger::instance().info("Cache hit: a -> " + value);
    }

    cache.put("c", "gamma"); // evicts "b"

    if (!cache.get("b", value)) {
        Logger::instance().info("Cache miss: b (evicted)");
    }

    Logger::instance().info("LRU cache test complete.");
    return 0;
}


