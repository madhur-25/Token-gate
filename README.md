# Token-Gate:C++ Rate Limiter

Token-Gate is a thread-safe rate limiter built in modern C++17. It uses the classic **Token Bucket** algorithm to control how often an action can be performed on a per-user basis. To prevent memory leaks in long-running services, it features a built-in **LRU Cache** that automatically evicts the least recently used user data.

The project is built using a clean, modular design and is fully verified with a suite of unit tests using Google Test.

---
## Key Features

* **Token Bucket Algorithm**: Implements a flexible, time-based rate limiting algorithm with configurable capacity and token refill rates.
* **Efficient Memory Management**: Uses a built-in LRU (Least Recently Used) Cache to manage a fixed number of user buckets, preventing memory leaks over time.
* **Thread-Safe by Design**: Built from the ground up to handle concurrent requests safely using C++ primitives like `std::mutex`, `std::lock_guard`, and `std::atomic`.
* **Clean & Modular Design**: Follows modern C++ best practices by separating concerns into logical components (Logger, Metrics, Cache, etc.) and organizing the project with a clean header file structure.
* **Unit Tested**: Includes a comprehensive test suite using the Google Test framework to ensure the logic is correct and reliable.
* **Cross-Platform Build System**: Uses CMake to manage the build process, making it easy to compile and run on any system.

---

## Technical Deep Dive

This project explores several key computer science and software engineering concepts:

* **System Design**: Explores fundamental backend concepts like rate limiting and caching (specifically LRU eviction strategy).
* **Data Structures**: A practical application of Hash Maps and Doubly Linked Lists to build an optimal O(1) LRU Cache.
* **Object-Oriented Programming**: Leverages polymorphism for different rate-limiting algorithms and RAII principles for resource management with smart pointers.
* **Concurrency**: A focus on multithreaded programming and ensuring safe access to shared data.
* **Modern C++ (17)**: Makes use of modern language features including smart pointers, the `<chrono>` library for precise timekeeping, and templates.

---

## Technology Stack

* **Language**: C++17
* **Build System**: CMake
* **Testing Framework**: Google Test

---

## Setup and Build Instructions

### Prerequisites
* Git
* CMake (v3.14+)
* A C++17 compliant compiler (GCC, Clang, or MSVC)

### Build Steps

1.  **Clone the repository:**
    ```bash
    git clone [https://github.com/madhur-25/Token-gate.git](https://github.com/madhur-25/Token-gate.git)
    cd Token-gate
    ```

2.  **Create a build directory:**
    ```bash
    mkdir build
    cd build
    ```

3.  **Configure and build the project:**
    ```bash
    cmake ..
    cmake --build .
    ```

4.  **Run the demo or the unit tests:**
    ```bash
    # Run the main demo showing the cache eviction
    ./main

    # Run the unit tests
    ./run_tests
    ```

---

## Demo Output

Running the `main` executable demonstrates the LRU cache evicting users when its capacity is reached.

```
--- Accessing 3 users to fill the cache ---
[INFO] 14:55:10 | user=user1 | Cache Miss. Creating new bucket.
[INFO] 14:55:10 | user=user1 | Allowed
[INFO] 14:55:10 | user=user2 | Cache Miss. Creating new bucket.
[INFO] 14:55:10 | user=user2 | Allowed
[INFO] 14:55:10 | user=user3 | Cache Miss. Creating new bucket.
[INFO] 14:55:10 | user=user3 | Allowed

--- Cache is now full. Order of recency: user3, user2, user1 ---
--- Accessing user1 again to make it the most recently used ---
[INFO] 14:55:10 | user=user1 | Cache Hit.
[INFO] 14:55:10 | user=user1 | Allowed

--- Accessing a new user (user4). This should evict the LRU user (user2) ---
[INFO] 14:55:10 | user=user4 | Cache Miss. Creating new bucket.
[INFO] 14:55:10 | user=user4 | Allowed

--- Accessing user2 again. It was evicted, so this should be a cache miss. ---
[INFO] 14:55:10 | user=user2 | Cache Miss. Creating new bucket.
[INFO] 14:55:10 | user=user2 | Allowed
```
