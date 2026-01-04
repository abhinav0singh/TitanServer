# TitanServer

TitanServer is a modern C++ server framework designed to demonstrate
systems-level programming concepts such as concurrency, memory safety,
and efficient data structures.

This project focuses on **core server architecture**, not frameworks.
It’s a simple web server written from scratch. It listens for connections, understands HTTP requests, decides what the user wants, and sends back responses. To keep it fast, it caches files in memory, and to handle multiple users, it uses a thread pool instead of spawning unlimited threads.
---

##  Features

- Fixed-size thread pool using `std::thread` and `condition_variable`
- Thread-safe logger (singleton, RAII-based)
- Thread-safe LRU cache with O(1) get/put
- Clean CMake-based build system
- Modern C++17 design

---

##  Architecture Overview

TitanServer is built in layers:

- **Infrastructure layer**
  - Logger
  - Thread Pool
  - Task Queue
- **Data layer**
  - LRU Cache
- **Application layer**
  - (HTTP parsing and networking – planned)

All components are designed with correctness, clarity, and extensibility
as primary goals.

---

##  Build Instructions

### Requirements
- CMake 3.20+
- C++17 compatible compiler
- Visual Studio (Windows)

### Build

```bash
cmake -S . -B build
cmake --build build
```


##  Interactive Dashboard

TitanServer includes a browser-based dashboard to visualize
request handling, routing, caching, and concurrency in real time.

Visit:
http://localhost:8080/dashboard
