# Technical Blueprint: C++23 Ultra-Low Latency Async Web Server

## 0. Core Concept: The Paradigm Shift (Reactor vs. Proactor)

### The Old Way: Reactor Pattern (epoll/kqueue/poll)
In the traditional `epoll` model (Reactor), the OS notifies you when a file descriptor is **ready** to be read or written.
1. **Wait**: You block on `epoll_wait`.
2. **Wake**: OS wakes you up: "Socket 5 is readable".
3. **Act**: You call `read(5, buffer)`. **This is a system call.** It does the work synchronously (even if non-blocking, the data copy happens now).
4. **Repeat**: High syscall overhead (one for wait, one for read/write).

### The New Way: Proactor Pattern (io_uring/IOCP)
In `io_uring` (Linux) and `IOCP` (Windows), you submit **operations**, not interest.
1. **Submit**: You tell the OS: "Please read from Socket 5 into this buffer and tell me when you are done."
2. **Wait/Check**: You check the Completion Queue (CQ).
3. **Complete**: The OS says: "Operation X is done. Data is already in the buffer."

**Why it wins:**
- **Fewer Syscalls**: `io_uring` allows batching submissions and completions. You can submit 10 reads and reap 10 completions in a single syscall (or zero syscalls with kernel polling).
- **True Async**: The data copy happens in the kernel background (or via DMA), not consuming your thread's cycles during the "read" call.

---

## Phase 1: The Event Loop (The Heart)

**Concept**: Thread-per-Core architecture with a Ring-based Event Loop.
Each thread is pinned to a CPU core and runs its own `io_uring` instance (or IOCP port). No mutexes, no cross-thread communication for I/O.

**Low Level Design**:
- **Submission Queue (SQ)**: Where we push `SQE` (Submission Queue Entries) - e.g., `IORING_OP_RECV`, `IORING_OP_ACCEPT`.
- **Completion Queue (CQ)**: Where we reap `CQE` (Completion Queue Entries).
- **UserData**: The critical link. When submitting an SQE, we set `sqe->user_data` to a pointer address of our `CoroutineHandle`. When the CQE arrives, we cast `cqe->user_data` back to the handle and resume execution.

**Implementation Goal**:
Create a `Ring` class that wraps `io_uring_queue_init` (or `CreateIoCompletionPort`).
It must expose a `submit_and_wait()` method that drives the loop.

---

## Phase 2: Memory Management (The Muscle)

**Concept**: Dynamic allocation (`new`/`malloc`) is the enemy of latency. We need **Stable Pointers** and **Cache Locality**.

**Structures**:
- **Connection Pool**: A pre-allocated `std::vector<Connection>` (or a Slab Allocator).
    - **Problem**: `std::vector` resize invalidates pointers.
    - **Solution**: Use a `std::deque` or a fixed-size `std::vector` reserved at startup. Or better, a custom `BlockAllocator`.
- **Buffer Pool**:
    - Each connection needs a read buffer.
    - Instead of `std::vector<char>`, use a monolithic memory block divided into 4KB chunks.
    - `FreeList`: A simple stack of indices pointing to free chunks.

**Design**:
```cpp
struct Connection {
    int fd;
    std::span<char> read_buffer;
    // State machine context
};
```
The `Connection` object must remain at a stable memory address for the duration of the session because `io_uring` needs the buffer address to remain valid until completion.

---

## Phase 3: Coroutine Integration (The Nervous System)

**Concept**: C++23 Coroutines turn "Callback Hell" into linear code. We need to make `io_uring` "Awaitable".

**The Challenge**:
`co_await socket.recv(buffer)` needs to:
1. **Suspend**: Pause the current function.
2. **Submit**: Push an `IORING_OP_RECV` to the ring with `user_data = coroutine_handle`.
3. **Resume**: When the CQE returns, the Event Loop calls `handle.resume()`.

**Low Level Structure**:
Define a custom Awaitable type:
```cpp
struct IOAwaitable {
    Ring* ring;
    int fd;
    void* buf;
    size_t len;
    int result;

    bool await_ready() { return false; } // Always suspend
    void await_suspend(std::coroutine_handle<> h) {
        // Register 'h' into the SQE's user_data
        ring->submit_recv(fd, buf, len, h);
    }
    int await_resume() { return result; } // Return bytes read
};
```

---

## Phase 4: HTTP Parser (The Brain)

**Concept**: Zero-Allocation, Zero-Copy Parsing.
We do not construct `std::string` or `std::map` for headers. We use `std::string_view` (or `std::span`) pointing directly into the raw Read Buffer.

**State Machine**:
Implement a DFA (Deterministic Finite Automaton) that processes byte-by-byte or word-by-word.
- **States**: `METHOD`, `URI`, `VERSION`, `HEADER_KEY`, `HEADER_VALUE`, `BODY`.
- **Output**: A `Request` struct containing `std::string_view`s.

**Optimization**:
- Use SIMD (SSE4.2/AVX2) to scan for `\r\n` and `:` delimiters if possible, or just optimized `memchr`.
- **Safety**: Handle partial reads. If a packet ends in the middle of a header, the state machine must pause and resume when more data arrives.

---

## Phase 5: Zero Copy I/O (The Speed)

**Concept**: The payload (e.g., `index.html`) should never touch User Space memory.

**Mechanism**:
- **Linux**: `IORING_OP_SPLICE` or `IORING_OP_SENDFILE`.
    - Transfers data from File Descriptor (Disk) -> Pipe -> Socket Descriptor (Network) entirely in kernel.
- **Windows**: `TransmitFile` (via IOCP).

**Flow**:
1. Parse Request -> Get File Path.
2. `open` file (async via `io_uring`).
3. Write HTTP Headers (from user buffer).
4. `co_await sendfile(socket_fd, file_fd, offset, size)`.
5. Close file.

---

## Proposed Project Structure

```text
root/
├── CMakeLists.txt
├── src/
│   ├── main.cpp              # Entry point, thread spawning
│   ├── core/
│   │   ├── Ring.hpp          # io_uring/IOCP wrapper
│   │   ├── Task.hpp          # Coroutine return type
│   │   └── BufferPool.hpp    # Memory management
│   ├── net/
│   │   ├── Socket.hpp        # RAII wrapper for fds
│   │   └── Acceptor.hpp      # Async accept loop
│   ├── http/
│   │   ├── Parser.hpp        # State machine
│   │   └── Request.hpp       # string_view based structs
│   └── sys/
│       ├── Syscall.hpp       # Native syscall wrappers
│       └── Platform.hpp      # Linux/Windows defines
└── tests/
    └── ...
```
