# Ferry Tour Simulation (CPU-Intensive, Multithreaded C Project)

This project is a **multithreaded ferry simulation** written in C using **POSIX threads (pthreads)**.  
It simulates cars, minibuses, and trucks passing through tolls, boarding a ferry, traveling between two sides, doing work on the other side, and then returning to their starting side.

In addition to concurrency and synchronization, the project also includes **CPU-intensive computations** to stress the processor and demonstrate performance under heavy load.

---

## 🎯 Features

- **Multiple vehicle types** with different sizes:
  - Cars (`TOTAL_CARS`)
  - Minibuses (`TOTAL_MINIBUSES`)
  - Trucks (`TOTAL_TRUCKS`)
- **Ferry with limited capacity** (`FERRY_CAPACITY`)
- Each vehicle:
  - Starts from a random side
  - Passes through a random toll on its side
  - Boards the ferry if there is enough capacity
  - Travels to the opposite side, performs heavy work
  - Boards the ferry again to return home
- **Ferry manager thread**:
  - Decides when the ferry should depart
  - Tracks load, departures and simulation progress
- **CPU-intensive mode**:
  - Heavy floating-point operations
  - String processing & hash-like operations
  - Busy waiting instead of regular sleep in some places
  - Configurable iterations via macros

---

## 🧮 Main Parameters

You can tweak the behavior of the simulation by editing the macros at the top of `ferry.c`:

```c
#define FERRY_CAPACITY 20

#define TOTAL_CARS 12
#define TOTAL_MINIBUSES 10
#define TOTAL_TRUCKS 8

#define CAR_UNIT 1
#define MINIBUS_UNIT 2
#define TRUCK_UNIT 3

// CPU intensive parameters
#define WORK_ITERATIONS 1000000
#define PROCESSING_DELAY 1000
#define BUSY_WAIT_CYCLES 500000

