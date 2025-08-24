#ifndef LRU_HPP
#define LRU_HPP

#include <list>
#include <unordered_map>
#include <utility> // for std::pair

//  LRU Cache
template <typename Key, typename Value> class LRUCache {
private:
  size_t capacity;
  std::list<std::pair<Key, Value>> cacheList; // keeps track of usage order
  std::unordered_map<Key, typename std::list<std::pair<Key, Value>>::iterator>
      cacheMap;

public:
  LRUCache(size_t cap) : capacity(cap) {}

  bool get(const Key &key, Value &value) {
    auto it = cacheMap.find(key);
    if (it == cacheMap.end()) {
      return false; // not found
    }

    // Move accessed node to the front (most recently used)
    cacheList.splice(cacheList.begin(), cacheList, it->second);
    value = it->second->second;
    return true;
  }

  void put(const Key &key, const Value &value) {
    auto it = cacheMap.find(key);

    if (it != cacheMap.end()) {
      // Update value
      it->second->second = value;
      // Move to front
      cacheList.splice(cacheList.begin(), cacheList, it->second);
    } else {
      if (cacheList.size() == capacity) {
        // Remove least recently used (last element)
        auto last = cacheList.back();
        cacheMap.erase(last.first);
        cacheList.pop_back();
      }
      // Insert new node at front
      cacheList.emplace_front(key, value);
      cacheMap[key] = cacheList.begin();
    }
  }

  void display() const {
    for (auto &pair : cacheList) {
      std::cout << pair.first << ":" << pair.second << " ";
    }
    std::cout << std::endl;
  }
};

#endif 
