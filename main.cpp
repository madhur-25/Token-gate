#include <iostream>
#include <thread>
#include <chrono>


#include "rate_limiter.hpp"

using namespace std;
using namespace std::chrono;

int main() {
    Logger logger;
    Metrics metrics;

    // Rate limiter with a cache that holds a maximum of 3 user buckets.
    RateLimiter limiter(5, 1, logger, metrics, 3);

    cout << "--- Accessing 3 users to fill the cache ---\n";
    limiter.isRequestAllowed("user1");
    this_thread::sleep_for(milliseconds(100));
    limiter.isRequestAllowed("user2");
    this_thread::sleep_for(milliseconds(100));
    limiter.isRequestAllowed("user3");

    cout << "\n--- Cache is now full. Order of recency: user3, user2, user1 ---\n";
    cout << "--- Accessing user1 again to make it the most recently used ---\n";
    limiter.isRequestAllowed("user1"); // Now order is: user1, user3, user2

    cout << "\n--- Accessing a new user (user4). This should evict the LRU user (user2) ---\n";
    limiter.isRequestAllowed("user4"); // Evicts user2. Order: user4, user1, user3

    cout << "\n--- Accessing user2 again. It was evicted, so this should be a cache miss. ---\n";
    limiter.isRequestAllowed("user2"); // Creates a new bucket for user2. Evicts user3.

    return 0;
}