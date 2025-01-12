# UserSched

## Overview

This project implements a Weighted Round Robin (WRR) scheduler for process management, simulating a multi-core CPU environment. The scheduler manages multiple users, each with varying weights, and schedules their processes using round-robin (RR) within the allocated time slices. The implementation is multi-threaded, allowing each CPU to run its own scheduler in parallel, ensuring scalability and efficiency.

## Features:

### WRR Scheduling:
- Users are assigned weights that determine their CPU time allocation.
- Processes within a user are scheduled using RR

### Multi-Core Simulation:
- Multiple CPUs are simulated using threads, each running an independent scheduler.

### Interuption handling
- The scheduler assures that resources are cleaned up and processes are terminated safely when an interruption occurs.

### Dynamic Process and User Management
- Users and their processes are dynamically created and managed.

### Synchronization
- Mutexes, condition variables and semaphores are used to ensure thread-saf access across shared resources.

## Workflow
### Initialization
- CPUs, users and processes are dynamically generated.
- Each CPU is assigned a set of users based on a randomized weight distribution.

### WRR Scheduling
- Users are scheduled in a round-robin fashion, with their time slices proportional to their weights.
- Processes within each user are scheduled using round-robin within the allocated time.

### Multi-Core Execution
- Each CPU runs its own scheduler thread, allowing parallel execution.

### Shutdown
- Signal handlers ensure all processes and resources are cleaned up when the program is interrupted.

## Code structure
### CPU Structure (```struct CPU```)
- Represents a single CPU in the system
- Contains a list of users and mantains runtime statistics.

### User Structure (```struct User```)
- Represents a user with associated processes
- Stores the user's weight, allocated time and process queues.

### Process Structure (```struct Process```)
- Represents a single process with execution and arrival times.

### Key Functions:
1. Initialization and Cleanup:
- ```init_sync``` : Initializes mutexes, condition variables, and semaphores.
- ```cleanup_sync```: Destroys synchronization primitives.

2. Process and User Management:
- ```generate_processes```: Dynamically creates processes for a user.
- ```generate_users```: Dynamically creates users and assigns them to a CPU.

3. Schedulers:
- ```rr_processes_scheduler```: Implements round-robin scheduling for processes within a user.
- ```wrr_users_scheduler```: Implements weighted round-robin scheduling across users.

4. Shutdown:
- ```handle_sigint```: Catches SIGINT and safely terminates all processes and threads.
- ```handle_sigchld```: Handles terminated child processes to prevent zombies.

5. Threaded CPU Simulation:
- ```cpu_scheduler_thread```: Runs the scheduler for a single CPU in a separate thread.

### Synchronization
- Mutexes: Protect access to shared data structures (user and process lists).
- Condition Variables: Signal when processes are ready for execution.
- Semaphores: Control access to CPU resources, ensuring only one process executes at a time per CPU.

## How to run scheduler
### Prerequisites:
- GCC or a compatible C compiler.
- POSIX-compilant operating system (Linux or macOS)

### Compilation:
```
gcc -pthread -o scheduler.c scheduler
```

### Execution:
```
./scheduler
```

Console output shows scheduler activity and process transitions.
