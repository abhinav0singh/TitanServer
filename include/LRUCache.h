#pragma once

#include <unordered_map>
#include <list>
#include <mutex>

template <typename Key, typename Value>
class LRUCache {
public:
    explicit LRUCache(size_t capacity)
        : capacity_(capacity) {
    }

    bool get(const Key& key, Value& value);
    void put(const Key& key, const Value& value);

private:
    using ListIt = typename std::list<std::pair<Key, Value>>::iterator;

    size_t capacity_;
    std::list<std::pair<Key, Value>> items_; // MRU at front
    std::unordered_map<Key, ListIt> map_;
    std::mutex mutex_;
};

template <typename Key, typename Value>
bool LRUCache<Key, Value>::get(const Key& key, Value& value) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = map_.find(key);
    if (it == map_.end()) {
        return false;
    }

    // Move accessed item to front (MRU)
    items_.splice(items_.begin(), items_, it->second);
    value = it->second->second;
    return true;
}

template <typename Key, typename Value>
void LRUCache<Key, Value>::put(const Key& key, const Value& value) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = map_.find(key);
    if (it != map_.end()) {
        // Update existing
        it->second->second = value;
        items_.splice(items_.begin(), items_, it->second);
        return;
    }

    // Insert new
    items_.emplace_front(key, value);
    map_[key] = items_.begin();

    if (map_.size() > capacity_) {
        // Evict LRU
        auto last = items_.end();
        --last;
        map_.erase(last->first);
        items_.pop_back();
    }
}
