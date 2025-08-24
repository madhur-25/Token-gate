#ifndef RATE_LIMITER_HPP
#define RATE_LIMITER_HPP

#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>


#include "lru_cache.hpp"

using namespace std;
using namespace std::chrono;



class Logger {
  mutex mtx;
  static string nowStr() {
    auto now = system_clock::now();
    time_t tt = system_clock::to_time_t(now);
    tm tmv{};
#ifdef _WIN32
    localtime_s(&tmv, &tt);
#else
    localtime_r(&tt, &tmv);
#endif
    ostringstream os;
    os << setfill('0') << setw(2) << tmv.tm_hour << ":" << setw(2) << tmv.tm_min
       << ":" << setw(2) << tmv.tm_sec;
    return os.str();
  }

public:
  template <typename... Args>
  void info(const string &user, const string &msg, Args &&...extra) {
    lock_guard<mutex> lock(mtx);
    cout << "[INFO] " << nowStr() << " | user=" << user << " | " << msg;
    ((cout << " | " << extra), ...);
    cout << "\n";
  }
  template <typename... Args>
  void warn(const string &user, const string &msg, Args &&...extra) {
    lock_guard<mutex> lock(mtx);
    cout << "[WARN] " << nowStr() << " | user=" << user << " | " << msg;
    ((cout << " | " << extra), ...);
    cout << "\n";
  }
};

struct Counters {
  atomic<long long> allowed{0}, blocked{0};
};

class Metrics {
  Counters global_;
  mutex mtx_;
  unordered_map<string, Counters> perUser_;

public:
  void recordAllowed(const string &user) {
    global_.allowed.fetch_add(1);
    lock_guard<mutex> lk(mtx_);
    perUser_[user].allowed.fetch_add(1);
  }
  void recordBlocked(const string &user) {
    global_.blocked.fetch_add(1);
    lock_guard<mutex> lk(mtx_);
    perUser_[user].blocked.fetch_add(1);
  }
};

class RateLimitAlgorithm {
public:
  virtual bool allowRequest() = 0;
  virtual ~RateLimitAlgorithm() = default;
};

class TokenBucket : public RateLimitAlgorithm {
  const double capacity;
  double tokens;
  const double refillRatePerSec;
  time_point<steady_clock> lastRefillTime;
  mutex mtx;
  void refillTokensLocked() {
    auto now = steady_clock::now();
    auto msPassed = duration_cast<milliseconds>(now - lastRefillTime).count();
    if (msPassed <= 0)
      return;
    double add = (msPassed / 1000.0) * refillRatePerSec;
    tokens = min(capacity, tokens + add);
    lastRefillTime = now;
  }

public:
  TokenBucket(int cap, int rate)
      : capacity(cap), tokens(cap), refillRatePerSec(rate),
        lastRefillTime(steady_clock::now()) {}
  bool allowRequest() override {
    lock_guard<mutex> lock(mtx);
    refillTokensLocked();
    if (tokens >= 1.0) {
      tokens -= 1.0;
      return true;
    }
    return false;
  }
};

class UserBucket {
  unique_ptr<RateLimitAlgorithm> algo;

public:
  UserBucket(int cap, int rate) : algo(make_unique<TokenBucket>(cap, rate)) {}
  bool allowRequest() { return algo->allowRequest(); }
};

// --- RateLimiter Class (Modified for Integration) ---
class RateLimiter {
  LRUCache<string, shared_ptr<UserBucket>> buckets;
  int capacity;
  int refillRatePerSec;
  mutex cacheMtx;
  Logger &logger;
  Metrics &metrics;

  shared_ptr<UserBucket> ensureBucket(const string &userId) {
    lock_guard<mutex> lock(cacheMtx);
    shared_ptr<UserBucket> user_bucket;
    if (buckets.get(userId, user_bucket)) {
      logger.info(userId, "Cache Hit.");
    } else {
      logger.info(userId, "Cache Miss. Creating new bucket.");
      user_bucket = make_shared<UserBucket>(capacity, refillRatePerSec);
      buckets.put(userId, user_bucket);
    }
    return user_bucket;
  }

public:
  RateLimiter(int bucket_cap, int refill_rate, Logger &L, Metrics &M,
              size_t cache_capacity)
      : buckets(cache_capacity), capacity(bucket_cap),
        refillRatePerSec(refill_rate), logger(L), metrics(M) {}

  bool isRequestAllowed(const string &userId) {
    auto ub = ensureBucket(userId);
    bool allowed = ub->allowRequest();
    if (allowed) {
      metrics.recordAllowed(userId);
      logger.info(userId, "Allowed");
    } else {
      metrics.recordBlocked(userId);
      logger.warn(userId, "Blocked");
    }
    return allowed;
  }
};

#endif 