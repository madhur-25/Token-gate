#include "lru_cache.hpp"
#include "rate_limiter.hpp" // rate limiter 
#include <gtest/gtest.h>    // Google Test framework
#include <thread>

// Test Case 1: Ensure the token bucket blocks requests when it's empty.
TEST(TokenBucketTest, BlocksWhenEmpty) {
  TokenBucket bucket(2, 1); // 2 tokens capacity

  // Use up the 2 tokens
  ASSERT_TRUE(bucket.allowRequest());
  ASSERT_TRUE(bucket.allowRequest());

  // The 3rd request should fail
  EXPECT_FALSE(bucket.allowRequest());
}

// Test Case 2: Ensure tokens refill over time.
TEST(TokenBucketTest, RefillsOverTime) {
  TokenBucket bucket(2, 1); // 2 tokens capacity, 1 token/sec refill

  // Use up all tokens
  ASSERT_TRUE(bucket.allowRequest());
  ASSERT_TRUE(bucket.allowRequest());
  ASSERT_FALSE(bucket.allowRequest()); // Bucket is now empty

  // Wait for 1.5 seconds, which should refill at least 1 token
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  // Now, a request should be allowed
  EXPECT_TRUE(bucket.allowRequest());
}

// Test Case 3: Ensure the main RateLimiter handles different users separately.
TEST(RateLimiterTest, SeparatesUsers) {
  Logger logger;
  Metrics metrics;
  RateLimiter limiter(1, 10, logger, metrics); // 1 request limit per user

  // userA should be allowed once, then blocked
  EXPECT_TRUE(limiter.isRequestAllowed("userA"));
  EXPECT_FALSE(limiter.isRequestAllowed("userA"));

  // userB's limit is separate, so their first request should be allowed
  EXPECT_TRUE(limiter.isRequestAllowed("userB"));
}

// Test Case 4: Check if metrics are recorded correctly.
TEST(MetricsTest, RecordsCorrectly) {
  Logger logger;
  Metrics metrics;
  RateLimiter limiter(1, 10, logger, metrics);

  // 1 allowed, 1 blocked for "user1"
  limiter.isRequestAllowed("user1");
  limiter.isRequestAllowed("user1");

  // Check the stats for "user1"
  auto [allowed, blocked] = metrics.user("user1");
  EXPECT_EQ(allowed, 1);
  EXPECT_EQ(blocked, 1);

  // Check the global stats
  auto [global_allowed, global_blocked] = metrics.global();
  EXPECT_EQ(global_allowed, 1);
  EXPECT_EQ(global_blocked, 1);
}