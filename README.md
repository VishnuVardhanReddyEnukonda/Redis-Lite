# Redis-Lite: High-Performance C++ Key-Value Store

A multithreaded, asynchronous, and persistent in-memory key-value database built entirely from scratch in C++. 

This project was engineered to replicate the core mechanics of enterprise distributed caches (like Redis or Memcached) to demonstrate low-level systems programming, network multiplexing, and thread-safe concurrency.

## 🚀 Core Architecture

* **Networking (Raw TCP & I/O Multiplexing):** Bypasses high-level HTTP libraries to manage raw TCP sockets. Utilizes `select()` for non-blocking I/O multiplexing, allowing a single thread to handle hundreds of concurrent client connections without freezing.
* **Concurrency (Thread Pool):** Implements a custom Thread Pool using `std::mutex` and `std::condition_variable`. The main event loop acts as a dispatcher, pushing network requests to a synchronized job queue where background worker threads process them with true parallelism.
* **Memory Management ($O(1)$ LRU Cache):** Data is managed in RAM using a custom-built Least Recently Used (LRU) Cache (Hash Map + Doubly Linked List), ensuring strict $O(1)$ time complexity for memory access and automatic eviction when capacity is reached.
* **Time-to-Live (TTL) Engine:** Features a lazy-eviction expiration engine that tracks timestamps in milliseconds and dynamically removes stale keys.
* **Persistence (AOF):** Implements an Append-Only File (AOF) storage layer. Every mutation is logged to the disk, guaranteeing data durability and allowing the database to rebuild its state directly into RAM after a complete system crash.
* **Protocol (RESP):** Communicates over the network using the official REdis Serialization Protocol (RESP), allowing the server to interact natively with standard Redis clients.

## 🛠️ Prerequisites

* A C++11 (or higher) compliant compiler (e.g., GCC/MinGW).
* Windows OS (Uses `<winsock2.h>` for networking). *Note: Can be easily adapted to POSIX `<sys/socket.h>` for Linux/macOS.*

## ⚙️ Build and Run

**1. Compile the Server:**
Open your terminal in the project directory and compile the source files, linking the Windows Socket library (`ws2_32`):
```bash
g++ server.cpp lru_cache.cpp -o redis_server.exe -lws2_32
