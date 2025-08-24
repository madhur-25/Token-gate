#include <iostream>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <iomanip>
#include <sstream>

using namespace std;
using namespace std::chrono;

// ---------- Logger (thread-safe) ----------
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
        os << setfill('0') << setw(2) << tmv.tm_hour << ":"
           << setw(2) << tmv.tm_min << ":" << setw(2) << tmv.tm_sec;
        return os.str();
    }
public:
    template <typename... Args>
    void info(const string& user, const string& msg, Args&&... extra) {
        lock_guard<mutex> lock(mtx);
        cout << "[INFO] " << nowStr() << " | user=" << user << " | " << msg;
        ((cout << " | " << extra), ...);
        cout << "\n";
    }
    template <typename... Args>
    void warn(const string& user, const string& msg, Args&&... extra) {
        lock_guard<mutex> lock(mtx);
        cout << "[WARN] " << nowStr() << " | user=" << user << " | " << msg;
        ((cout << " | " << extra), ...);
        cout << "\n";
    }
};

// ---------- Metrics ----------
struct Counters {
    atomic<long long> allowed{0};
    atomic<long long> blocked{0};
};
class Metrics {
    Counters global_;
    mutex mtx_; // protects map
    unordered_map<string, Counters> perUser_;
public:
    void recordAllowed(const string& user) {
        global_.allowed.fetch_add(1, memory_order_relaxed);
        lock_guard<mutex> lk(mtx_);
        perUser_[user].allowed.fetch_add(1, memory_order_relaxed);
    }
    void recordBlocked(const string& user) {
        global_.blocked.fetch_add(1, memory_order_relaxed);
        lock_guard<mutex> lk(mtx_);
        perUser_[user].blocked.fetch_add(1, memory_order_relaxed);
    }
    pair<long long,long long> global() const {
        return {global_.allowed.load(), global_.blocked.load()};
    }
    pair<long long,long long> user(const string& user) {
        lock_guard<mutex> lk(mtx_);
        auto it = perUser_.find(user);
        if (it == perUser_.end()) return {0,0};
        return {it->second.allowed.load(), it->second.blocked.load()};
    }
};

// ---------- Algorithms ----------
class RateLimitAlgorithm {
public:
    virtual bool allowRequest() = 0;
    virtual double approxTokens() = 0; // for logging/metrics
    virtual ~RateLimitAlgorithm() = default;
};

class TokenBucket : public RateLimitAlgorithm {
    const double capacity;
    double tokens;
    const double refillRatePerSec; // tokens per second
    time_point<steady_clock> lastRefillTime;
    mutex mtx;
    void refillTokensLocked() {
        auto now = steady_clock::now();
        auto msPassed = duration_cast<milliseconds>(now - lastRefillTime).count();
        if (msPassed <= 0) return;
        double add = (msPassed / 1000.0) * refillRatePerSec;
        tokens = min(capacity, tokens + add);
        lastRefillTime = now;
    }
public:
    TokenBucket(int cap, int ratePerSec)
        : capacity(cap), tokens(cap), refillRatePerSec(ratePerSec),
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
    double approxTokens() override {
        lock_guard<mutex> lock(mtx);
        // bring tokens up-to-date before reporting
        refillTokensLocked();
        return tokens;
    }
};

// ---------- User wrapper ----------
class UserBucket {
    unique_ptr<RateLimitAlgorithm> algo;
public:
    UserBucket(int capacity, int refillRatePerSec) {
        algo = make_unique<TokenBucket>(capacity, refillRatePerSec);
    }
    bool allowRequest() { return algo->allowRequest(); }
    double tokens() { return algo->approxTokens(); }
};

// ---------- RateLimiter ----------
class RateLimiter {
    unordered_map<string, shared_ptr<UserBucket>> buckets;
    int capacity;
    int refillRatePerSec;
    mutex mapMtx;

    Logger& logger;
    Metrics& metrics;

    shared_ptr<UserBucket> ensureBucket(const string& userId) {
        lock_guard<mutex> lk(mapMtx);
        auto it = buckets.find(userId);
        if (it == buckets.end()) {
            auto ub = make_shared<UserBucket>(capacity, refillRatePerSec);
            buckets[userId] = ub;
            return ub;
        }
        return it->second;
    }
public:
    RateLimiter(int capacity, int refillRatePerSec, Logger& L, Metrics& M)
        : capacity(capacity), refillRatePerSec(refillRatePerSec), logger(L), metrics(M) {}

    bool isRequestAllowed(const string& userId) {
        auto ub = ensureBucket(userId);
        bool allowed = ub->allowRequest();
        double tok = ub->tokens(); // approximate, post-decision

        if (allowed) {
            metrics.recordAllowed(userId);
            logger.info(userId, "Allowed", string("tokens=") + to_string(tok));
        } else {
            metrics.recordBlocked(userId);
            logger.warn(userId, "Blocked", string("tokens=") + to_string(tok));
        }
        return allowed;
    }
};

// ---------- Demo ----------
int main() {
    Logger logger;
    Metrics metrics;

    // capacity=5 tokens, refill=1 token/sec (millisecond precision)
    RateLimiter limiter(5, 1, logger, metrics);
    string user = "user123";

    cout << "Sending 7 rapid requests (300ms apart):\n";
    for (int i = 1; i <= 7; ++i) {
        bool allowed = limiter.isRequestAllowed(user);
        cout << "Request " << i << ": " << (allowed ? "Allowed" : "Blocked") << "\n";
        this_thread::sleep_for(milliseconds(300));
    }

    cout << "\nWaiting 3 seconds to refill tokens...\n";
    this_thread::sleep_for(seconds(3));

    cout << "Sending 3 more requests:\n";
    for (int i = 1; i <= 3; ++i) {
        bool allowed = limiter.isRequestAllowed(user);
        cout << "Request " << i << ": " << (allowed ? "Allowed" : "Blocked") << "\n";
    }

    auto [ga, gb] = metrics.global();
    auto [ua, ub] = metrics.user(user);
    cout << "\n--- Metrics ---\n";
    cout << "Global:   allowed=" << ga << " blocked=" << gb << "\n";
    cout << "Per-user: allowed=" << ua << " blocked=" << ub << "\n";

    return 0;
} 