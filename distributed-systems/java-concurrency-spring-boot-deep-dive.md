# Complete Java Concurrency & Spring Boot Internals Deep Dive

*A highly detailed, comprehensive guide for Fullstack Software Engineers (Java 25 / Spring Boot) focusing on distributed systems context, highly concurrent applications, and internal mechanics.*

---

## Part 1: Java Concurrency Fundamentals (Distributed Systems Context)

### 1. Thread Lifecycle, States, and Transitions

#### Detailed Explanation
In Java, a `Thread` is the basic unit of execution. The JVM maps these Java threads to native operating system (OS) threads (except in the case of Virtual Threads, introduced as a preview in Java 19 and finalized in Java 21, which are mapped to carrier threads). Managing thread states efficiently is critical for high-throughput distributed systems.

#### State Transitions Diagram
```ascii
                                         +-------------------+
                                         |                   |
                                         |      NEW          |
                                         |                   |
                                         +---------+---------+
                                                   |
                                                   | start()
                                                   v
+-----------------------+                +-------------------+                +-----------------------+
|                       |  yield()       |                   |   sleep(t)     |                       |
|    READY TO RUN       |<---------------+    RUNNABLE       +--------------->|   TIMED_WAITING       |
|                       |                |                   |                |                       |
+----------+------------+                +---------+---------+                +-----------+-----------+
           |                                       |                                      |
           | OS Scheduler                          | wait()                               | timeout
           |                                       |                                      | or notify()
           v                                       v                                      v
+-----------------------+                +-------------------+                +-----------------------+
|                       |                |                   |                |                       |
|      RUNNING          +--------------->|     WAITING       +--------------->|    READY TO RUN       |
|                       |                |                   |  notifyAll()   |                       |
+----------+------------+                +-------------------+                +-----------------------+
           |
           | blocked on I/O or lock
           v
+-----------------------+
|                       |
|       BLOCKED         |
|                       |
+----------+------------+
           | lock acquired
           v
+-----------------------+
|                       |
|      READY TO RUN     |
|                       |
+-----------------------+
           |
           | run() completes
           v
+-----------------------+
|                       |
|     TERMINATED        |
|                       |
+-----------------------+
```

#### Full Working Code Example
```java
public class ThreadLifecycleDemo {
    public static void main(String[] args) throws InterruptedException {
        Thread thread = new Thread(() -> {
            try {
                System.out.println("Thread state inside run(): " + Thread.currentThread().getState());
                Thread.sleep(1000);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        });

        System.out.println("State after creation: " + thread.getState()); // NEW
        thread.start();
        System.out.println("State after start: " + thread.getState()); // RUNNABLE

        // Give it a moment to hit sleep()
        Thread.sleep(100);
        System.out.println("State during sleep: " + thread.getState()); // TIMED_WAITING

        thread.join(); // Wait for it to finish
        System.out.println("State after completion: " + thread.getState()); // TERMINATED
    }
}
```

#### Common Mistakes and Anti-Patterns
- **Calling `run()` instead of `start()`**: This executes the task on the current thread (synchronously), completely defeating the purpose of multithreading.
- **Catching `InterruptedException` and ignoring it**: Always restore the interrupt status via `Thread.currentThread().interrupt()` if you catch it but don't propagate it.
- **Busy Waiting**: Using a `while(true)` loop to wait for a condition wastes CPU cycles. Use `wait()`/`notify()` or `CountDownLatch`.

#### Interview Q&A
**Q1: What is the difference between `BLOCKED` and `WAITING`?**
A: `BLOCKED` means the thread is trying to acquire an intrinsic monitor lock (`synchronized`) and cannot because another thread holds it. `WAITING` means the thread called `Object.wait()`, `Thread.join()`, or `LockSupport.park()` and is waiting for another thread to perform a specific action (like `notify()`).

**Q2: Can you restart a thread that has TERMINATED?**
A: No, once a thread reaches the `TERMINATED` state, calling `start()` again will throw an `IllegalThreadStateException`.

**Q3: How does `yield()` work and when should you use it?**
A: `Thread.yield()` is a hint to the scheduler that the current thread is willing to yield its current use of a processor. The scheduler is free to ignore this hint. It is rarely used in application code and is mainly used for debugging or testing race conditions.

**Q4: What happens if an exception is thrown inside a thread?**
A: If it's an unchecked exception, the thread terminates abruptly. You can handle this by setting an `UncaughtExceptionHandler` on the thread.

**Q5: How does thread interruption work?**
A: Interruption is a cooperative mechanism. Calling `thread.interrupt()` sets a flag. Blocking methods like `sleep()` or `wait()` check this flag and throw `InterruptedException`. Non-blocking code must manually check `Thread.currentThread().isInterrupted()`.

### 2. `synchronized` Keyword

#### Detailed Explanation
The `synchronized` keyword provides a mechanism for exclusive locking (mutual exclusion) in Java. It uses the intrinsic monitor lock associated with every Java object. Only one thread can execute a block of code synchronized on a specific object at any given time.

#### Monitor Lock ASCII Diagram
```ascii
Thread A                   Thread B                   Thread C
   |                          |                          |
   v                          v                          v
+-------------+            +-------------+            +-------------+
| Requests    |            | Requests    |            | Requests    |
| Lock        |            | Lock        |            | Lock        |
+-------------+            +-------------+            +-------------+
       |                          |                          |
       v                          v                          v
+---------------------------------------------------------------+
|                        Monitor Queue                          |
|  [ Thread A ] (acquires lock)                                 |
|  [ Thread B ] (WAITING)                                       |
|  [ Thread C ] (WAITING)                                       |
+---------------------------------------------------------------+
       |
       | executes critical section
       v
+---------------------------------------------------------------+
|  Counter.count++                                              |
+---------------------------------------------------------------+
       |
       | releases lock
       v
+---------------------------------------------------------------+
| Thread B or C is notified and acquires the lock               |
+---------------------------------------------------------------+
```

#### Full Working Code Example
```java
public class CounterSynchronized {
    private int count = 0;

    // Object-level lock (locks 'this' instance)
    public synchronized void increment() {
        count++;
    }
    
    // Equivalent block-level synchronization
    public void incrementBlock() {
        synchronized(this) {
            count++;
        }
    }

    // Class-level lock (locks CounterSynchronized.class)
    public static synchronized void printSystemStatus() {
        System.out.println("System is running properly");
    }

    public static void main(String[] args) throws InterruptedException {
        CounterSynchronized counter = new CounterSynchronized();
        Thread t1 = new Thread(() -> { for(int i=0; i<10000; i++) counter.increment(); });
        Thread t2 = new Thread(() -> { for(int i=0; i<10000; i++) counter.increment(); });

        t1.start(); t2.start();
        t1.join(); t2.join();

        System.out.println("Final Count (Should be 20000): " + counter.count);
    }
}
```

#### Common Mistakes and Anti-Patterns
- **Synchronizing on non-final fields**: If the object reference changes, threads might synchronize on different objects, destroying mutual exclusion.
- **Synchronizing on Strings or Wrapper classes**: E.g., `synchronized("LOCK")`. Due to the string pool, this can cause deadlocks if other parts of the system synchronize on the same literal.
- **Over-synchronization**: Synchronizing entire large methods when only a small block modifies shared state, degrading performance.

#### Interview Q&A
**Q1: What is the difference between object-level and class-level locks?**
A: Object-level locks synchronize on the instance (`this`). Multiple threads can simultaneously execute synchronized instance methods on *different* instances. Class-level locks synchronize on the `Class` object. No matter how many instances exist, only one thread can execute a static synchronized method for that class.

**Q2: Is `synchronized` reentrant?**
A: Yes. A thread that already holds a lock on an object can enter another synchronized block/method on the same object without blocking. The JVM keeps a lock count.

**Q3: Can a thread hold multiple locks simultaneously?**
A: Yes, it can enter nested synchronized blocks on different objects. However, this increases the risk of deadlocks if lock ordering is not strictly maintained.

**Q4: How does `synchronized` affect memory visibility?**
A: It provides a "happens-before" guarantee. Releasing a lock happens-before acquiring the same lock. All memory writes made by the thread releasing the lock are visible to the thread acquiring the lock.

**Q5: What are the performance implications of `synchronized` in modern Java?**
A: In older Java versions, it was slow due to OS-level context switching (heavyweight locks). Modern JVMs use biased locking and lightweight locking (spin locks) to minimize overhead for uncontended locks.

### 3. `volatile` Keyword

#### Detailed Explanation
The `volatile` keyword guarantees visibility of changes to variables across threads. When a field is declared `volatile`, the compiler and runtime are instructed that this variable is shared and operations on it should not be reordered with other memory operations. It forces threads to read the value directly from main memory (bypassing CPU caches) and write directly to main memory. **It does NOT guarantee atomicity.**

#### Memory Visibility Diagram
```ascii
WITHOUT VOLATILE:
+---------------+       +---------------+
| Thread 1 Cache|       | Thread 2 Cache|
| flag = true   |       | flag = false  |
+-------+-------+       +-------+-------+
        |                       |
        | (May flush later)     | (May read later)
        v                       v
+---------------------------------------+
|             MAIN MEMORY               |
|             flag = false              |
+---------------------------------------+

WITH VOLATILE:
+---------------+       +---------------+
| Thread 1 Cache|       | Thread 2 Cache|
| (Bypassed)    |       | (Bypassed)    |
+-------+-------+       +-------+-------+
        |                       |
        | (Immediate write)     | (Immediate read)
        v                       v
+---------------------------------------+
|             MAIN MEMORY               |
|             flag = true               |
+---------------------------------------+
```

#### Full Working Code Example
```java
public class VolatileDemo {
    private static volatile boolean shutdownRequested = false;

    public static void main(String[] args) throws InterruptedException {
        Thread worker = new Thread(() -> {
            long count = 0;
            // Without volatile, this loop might never see shutdownRequested = true
            while (!shutdownRequested) {
                count++;
            }
            System.out.println("Worker stopped. Counted up to: " + count);
        });

        worker.start();
        Thread.sleep(100); // Let the worker run for a bit
        
        System.out.println("Requesting shutdown...");
        shutdownRequested = true; // Visibility is guaranteed immediately to the worker
        
        worker.join();
        System.out.println("Main thread terminating.");
    }
}
```

#### Common Mistakes and Anti-Patterns
- **Assuming `volatile` means thread-safe operations**: Writing `volatile int count; count++;` is a race condition because `count++` is read-modify-write (3 steps), not atomic.
- **Using `volatile` for complex objects**: `volatile` on an array or object reference only guarantees the visibility of the reference itself, not the fields inside the object or elements inside the array.

#### Interview Q&A
**Q1: What is the "happens-before" guarantee of `volatile`?**
A: A write to a volatile variable happens-before any subsequent read of that volatile variable. This means any non-volatile writes before the volatile write are also visible after the volatile read.

**Q2: When should you use `volatile` instead of `AtomicBoolean`?**
A: If a variable is only written by one thread and read by many (like a state flag or shutdown signal), `volatile` is sufficient and slightly lighter. If multiple threads can conditionally update it (e.g., compare-and-swap), you must use `AtomicBoolean`.

**Q3: Can `volatile` prevent deadlocks?**
A: No, `volatile` has nothing to do with locking, so it cannot cause or prevent deadlocks.

**Q4: How does `volatile` prevent instruction reordering?**
A: The JVM inserts memory barriers (fences) around volatile reads and writes, preventing the CPU and compiler from reordering instructions across the barrier.

### 4. `ReentrantLock` vs `synchronized`

#### Detailed Explanation
`ReentrantLock` implements the `java.util.concurrent.locks.Lock` interface. It provides the same basic behavior and semantics as the implicit monitor lock accessed using `synchronized`, but with extended capabilities:
- **Fairness**: Can be instantiated as a "fair" lock (`new ReentrantLock(true)`), ensuring the longest-waiting thread gets the lock next.
- **Interruptible Lock Acquisition**: `lockInterruptibly()` allows a thread to respond to interrupts while waiting for a lock.
- **Non-blocking/Timed Attempts**: `tryLock()` and `tryLock(timeout, unit)` allow attempting to acquire a lock without blocking indefinitely.

#### Full Working Code Example
```java
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import java.util.concurrent.TimeUnit;

public class ReentrantLockDemo {
    private final Lock lock = new ReentrantLock(true); // Fair lock
    private int sharedResource = 0;

    public void performTask() {
        try {
            // Try to acquire lock, wait up to 1 second
            if (lock.tryLock(1, TimeUnit.SECONDS)) {
                try {
                    System.out.println(Thread.currentThread().getName() + " acquired the lock.");
                    sharedResource++;
                    Thread.sleep(500); // Simulate work
                } finally {
                    lock.unlock(); // ALWAYS unlock in a finally block!
                    System.out.println(Thread.currentThread().getName() + " released the lock.");
                }
            } else {
                System.out.println(Thread.currentThread().getName() + " could not acquire the lock (timeout).");
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }

    public static void main(String[] args) throws InterruptedException {
        ReentrantLockDemo demo = new ReentrantLockDemo();
        
        Runnable task = demo::performTask;
        
        Thread t1 = new Thread(task, "Thread-1");
        Thread t2 = new Thread(task, "Thread-2");
        Thread t3 = new Thread(task, "Thread-3");

        t1.start();
        t2.start();
        t3.start();

        t1.join(); t2.join(); t3.join();
    }
}
```

#### Common Mistakes and Anti-Patterns
- **Forgetting to unlock**: If an exception is thrown and `unlock()` is not in a `finally` block, the lock is held forever, causing deadlocks.
- **Calling `unlock()` without holding the lock**: Throws `IllegalMonitorStateException`.
- **Overusing fair locks**: Fair locks have significantly lower throughput than unfair locks due to the overhead of context switching and queue management. Only use fairness if absolute ordering is required.

#### Interview Q&A
**Q1: What does "Reentrant" mean?**
A: It means a thread that already holds the lock can acquire it again without blocking. The lock maintains a hold count. It must be unlocked the same number of times it was locked.

**Q2: When would you use `ReentrantLock` over `synchronized`?**
A: When you need advanced features: polling for locks (`tryLock`), interruptible lock waits, fairness, or when the locking block spans multiple methods (though that's usually bad design).

**Q3: How does `Condition` relate to `ReentrantLock`?**
A: `Condition` provides similar functionality to `Object.wait()` and `notify()`, but is bound to a specific `Lock`. You can have multiple `Condition` variables for a single `Lock` (e.g., `notFull` and `notEmpty` for a bounded buffer).

### 5. `ReadWriteLock`

#### Detailed Explanation
A `ReadWriteLock` maintains a pair of associated locks: one for read-only operations and one for writing. The read lock may be held simultaneously by multiple reader threads, as long as there are no writers. The write lock is exclusive.

#### Read-Write State ASCII Diagram
```ascii
State: No Locks Held
[R] -> OK, [W] -> OK

State: Read Lock Held (by Thread A, B)
[R] -> OK (Thread C can acquire)
[W] -> BLOCKED (Thread D must wait until A, B release)

State: Write Lock Held (by Thread E)
[R] -> BLOCKED
[W] -> BLOCKED
```

#### Full Working Code Example
```java
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;
import java.util.HashMap;
import java.util.Map;

public class CacheWithReadWriteLock {
    private final Map<String, String> cache = new HashMap<>();
    private final ReadWriteLock rwLock = new ReentrantReadWriteLock();

    public void put(String key, String value) {
        rwLock.writeLock().lock();
        try {
            System.out.println(Thread.currentThread().getName() + " writing.");
            cache.put(key, value);
            Thread.sleep(100); // Simulate slow write
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        } finally {
            rwLock.writeLock().unlock();
        }
    }

    public String get(String key) {
        rwLock.readLock().lock();
        try {
            System.out.println(Thread.currentThread().getName() + " reading.");
            Thread.sleep(50); // Simulate read delay
            return cache.get(key);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            return null;
        } finally {
            rwLock.readLock().unlock();
        }
    }

    // ... main method spawning multiple reader and writer threads ...
}
```

#### Interview Q&A
**Q1: Can a thread holding a read lock acquire the write lock?**
A: No, that would lead to a deadlock. It must release the read lock first.

**Q2: Can a thread holding the write lock acquire the read lock?**
A: Yes. This is called "lock downgrading". It can acquire the read lock and then release the write lock, holding onto the read lock.

**Q3: Is `ReadWriteLock` always faster than `ReentrantLock`?**
A: No. `ReadWriteLock` has higher internal complexity. It only outperforms an exclusive lock in scenarios with very high read-to-write ratios and long-duration read operations.

### 6. `Atomic*` Classes & CAS Operations

#### Detailed Explanation
The `java.util.concurrent.atomic` package provides lock-free, thread-safe programming on single variables. It relies on the hardware-level Compare-And-Swap (CAS) instruction. CAS checks if a memory location has a specific expected value; if it does, it updates it to a new value. If not, the operation fails (and usually loops/retries).

#### CAS Process Diagram
```ascii
Thread A Wants to increment AtomicInteger (Current value: 5)
1. Read current value (5).
2. Calculate new value (6).
3. CAS(expected=5, new=6)
   -> If memory still holds 5, atomically set to 6. Success!
   -> If memory holds 7 (changed by Thread B), CAS fails.
4. If fail, loop back to step 1.
```

#### Full Working Code Example
```java
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

public class AtomicDemo {
    private final AtomicInteger counter = new AtomicInteger(0);
    
    // Custom object atomic reference
    static class Node { String data; Node next; }
    private final AtomicReference<Node> head = new AtomicReference<>(null);

    public void increment() {
        // Uses CAS under the hood
        counter.incrementAndGet();
    }
    
    // Implementing a lock-free stack push using AtomicReference
    public void push(String data) {
        Node newNode = new Node();
        newNode.data = data;
        
        while (true) {
            Node currentHead = head.get();
            newNode.next = currentHead;
            // CAS: If head is still currentHead, set it to newNode. Else retry.
            if (head.compareAndSet(currentHead, newNode)) {
                break; // Success
            }
        }
    }
}
```

#### Interview Q&A
**Q1: What is the ABA problem?**
A: Thread 1 reads value A. Thread 2 changes it to B, then back to A. Thread 1 performs CAS expecting A, and succeeds. However, the state *did* change, which might be critical (e.g., in linked nodes). 

**Q2: How do you solve the ABA problem?**
A: Use `AtomicStampedReference`, which holds both an object reference and an integer "stamp" (version). Both must match for the CAS to succeed.

### 7. `ThreadLocal`

#### Detailed Explanation
`ThreadLocal` provides thread-local variables. These variables differ from their normal counterparts in that each thread that accesses one has its own, independently initialized copy of the variable.

#### Memory Leak Danger in Thread Pools
In distributed systems (like Spring Boot on Tomcat), threads are pooled. If a `ThreadLocal` is set but not removed (`.remove()`), the data persists for the life of the thread. When the thread handles a *new* request, it might see the old data (security breach) or cause memory leaks (OOM) because the object cannot be garbage collected.

#### Full Working Code Example
```java
public class UserContextHolder {
    private static final ThreadLocal<String> currentUser = new ThreadLocal<>();

    public static void set(String user) {
        currentUser.set(user);
    }

    public static String get() {
        return currentUser.get();
    }

    public static void clear() {
        currentUser.remove(); // CRITICAL TO AVOID LEAKS
    }
}

// Example Filter in Spring Boot
/*
public void doFilter(Request req, Response res, FilterChain chain) {
    try {
        UserContextHolder.set(extractUser(req));
        chain.doFilter(req, res);
    } finally {
        UserContextHolder.clear(); // Always clean up in finally!
    }
}
*/
```


---

## Part 2: java.util.concurrent Deep Dive

### 1. `ExecutorService` & Thread Pools

#### Full Code Examples for ALL Pool Types

```java
import java.util.concurrent.*;

public class ExecutorServiceDemo {
    public static void main(String[] args) {
        // 1. Fixed Thread Pool: Fixed size, unbounded queue.
        // Risk: Queue can grow infinitely if tasks are submitted faster than processed (OOM).
        ExecutorService fixedPool = Executors.newFixedThreadPool(10);
        
        // 2. Cached Thread Pool: Synchronous queue (size 0), infinite max threads.
        // Idle threads die after 60s.
        // Risk: Can spawn thousands of threads leading to OS thread exhaustion.
        ExecutorService cachedPool = Executors.newCachedThreadPool();
        
        // 3. Scheduled Thread Pool: For delayed or periodic execution.
        ScheduledExecutorService scheduledPool = Executors.newScheduledThreadPool(5);
        scheduledPool.scheduleAtFixedRate(() -> System.out.println("Ping"), 0, 1, TimeUnit.SECONDS);
        
        // 4. Single Thread Executor: Guarantees sequential execution of tasks.
        ExecutorService singlePool = Executors.newSingleThreadExecutor();
        
        // 5. Work Stealing Pool (Java 8+): Backed by ForkJoinPool.
        ExecutorService workStealing = Executors.newWorkStealingPool();

        // Custom, safe ThreadPool (What you should actually use in production!)
        ThreadPoolExecutor customPool = new ThreadPoolExecutor(
            5, // core pool size
            20, // max pool size
            60L, TimeUnit.SECONDS, // keep alive time
            new ArrayBlockingQueue<>(100), // Bounded queue!
            new ThreadPoolExecutor.CallerRunsPolicy() // Rejection policy
        );
    }
}
```

#### Interview Q&A
**Q1: What is the `CallerRunsPolicy`?**
A: When the queue is full and max threads are reached, instead of throwing a `RejectedExecutionException` (default), the task is executed synchronously by the thread that submitted it. This acts as a back-pressure mechanism.

**Q2: Describe the ThreadPoolExecutor growth strategy.**
A: 
1. New task arrives -> if threads < corePoolSize, create thread.
2. If threads == corePoolSize -> put task in queue.
3. If queue is full -> if threads < maxPoolSize, create thread.
4. If threads == maxPoolSize -> reject task.

### 2. `CompletableFuture`

#### Detailed Explanation
`CompletableFuture` allows for asynchronous, non-blocking composition of tasks. It is far superior to standard `Future` because you don't have to call `.get()` (which blocks).

#### Full Chain Example
```java
import java.util.concurrent.CompletableFuture;

public class CompletableFutureDemo {
    
    public static void main(String[] args) {
        System.out.println("Main thread: " + Thread.currentThread().getName());

        CompletableFuture<String> future = CompletableFuture.supplyAsync(() -> {
            System.out.println("Fetching user... thread: " + Thread.currentThread().getName());
            sleep(500);
            return "User123";
        })
        .thenApplyAsync(user -> { // map equivalent
            System.out.println("Enriching user... thread: " + Thread.currentThread().getName());
            return user + " - Admin";
        })
        .exceptionally(ex -> {
            System.out.println("Error occurred: " + ex.getMessage());
            return "FallbackUser";
        });

        // thenCompose is flatMap equivalent
        CompletableFuture<String> orderFuture = future.thenCompose(user -> fetchOrdersAsync(user));
        
        // allOf example
        CompletableFuture<Void> allDone = CompletableFuture.allOf(future, orderFuture);
        allDone.join(); // Block until all are complete
    }

    static CompletableFuture<String> fetchOrdersAsync(String user) {
        return CompletableFuture.supplyAsync(() -> "Orders for " + user);
    }

    static void sleep(int ms) {
        try { Thread.sleep(ms); } catch (Exception e) {}
    }
}
```

### 3. Synchronizers

#### `CountDownLatch` Full Example
Wait for multiple services to initialize before accepting requests.
```java
import java.util.concurrent.CountDownLatch;

public class LatchDemo {
    public static void main(String[] args) throws InterruptedException {
        CountDownLatch latch = new CountDownLatch(3);
        
        for(int i=0; i<3; i++) {
            new Thread(() -> {
                System.out.println("Service initialized.");
                latch.countDown();
            }).start();
        }
        
        latch.await(); // Blocks until count is 0
        System.out.println("All services up. Accepting traffic.");
    }
}
```

#### `CyclicBarrier` Full Example
Wait for threads to reach a barrier, run an action, then reset.
```java
import java.util.concurrent.CyclicBarrier;

public class BarrierDemo {
    public static void main(String[] args) {
        CyclicBarrier barrier = new CyclicBarrier(3, () -> System.out.println("All threads arrived. Proceeding to next phase."));
        
        for(int i=0; i<3; i++) {
            new Thread(() -> {
                try {
                    System.out.println("Phase 1 work...");
                    barrier.await();
                    System.out.println("Phase 2 work...");
                } catch (Exception e) {}
            }).start();
        }
    }
}
```

### 4. `ConcurrentHashMap` Internal Architecture

#### Detailed Explanation
`ConcurrentHashMap` provides high concurrency for reads and thread-safe writes.
- **Java 7**: Used Segmented Arrays (lock striping). It divided the map into e.g., 16 segments. Only the segment containing the key was locked on write.
- **Java 8+**: Segments are gone. It uses an array of node bins (like standard `HashMap`). For updates, it uses CAS operations. If there's a collision in a bin, it synchronizes on the *first node* (head) of that specific bin.
- Read operations are lock-free and use `volatile` reads.

### 5. `ForkJoinPool`

#### Detailed Explanation
Designed for divide-and-conquer algorithms. It uses a "work-stealing" algorithm: if a worker thread's deque is empty, it steals a task from the tail of another worker's deque.

#### Work Stealing Diagram
```ascii
Thread 1 Deque: [ TaskA ] [ TaskB ] [ TaskC ]  <-- Thread 1 processes from head
Thread 2 Deque: [ ] (Empty) 
                 |
                 +--> Steals [ TaskC ] from Thread 1's tail
```

---

## Part 3: Spring Boot Internals Deep Dive

### 1. Spring Bean Lifecycle Diagram

```ascii
+---------------------------------------------------+
| 1. Instantiation (Constructor called)             |
+-------------------------+-------------------------+
                          |
+-------------------------v-------------------------+
| 2. Populate Properties (Dependency Injection)     |
+-------------------------+-------------------------+
                          |
+-------------------------v-------------------------+
| 3. Aware Interfaces (e.g., ApplicationContextAware|
+-------------------------+-------------------------+
                          |
+-------------------------v-------------------------+
| 4. BeanPostProcessor.postProcessBeforeInit()      |
+-------------------------+-------------------------+
                          |
+-------------------------v-------------------------+
| 5. Initialization Callbacks (@PostConstruct)      |
+-------------------------+-------------------------+
                          |
+-------------------------v-------------------------+
| 6. BeanPostProcessor.postProcessAfterInit()       | <-- Proxies (AOP/Transactional) created here!
+-------------------------+-------------------------+
                          |
+-------------------------v-------------------------+
| 7. Bean is ready for use                          |
+-------------------------+-------------------------+
                          |
+-------------------------v-------------------------+
| 8. Destruction Callbacks (@PreDestroy)            |
+---------------------------------------------------+
```

### 2. `@Transactional` Proxy Mechanism Step-by-Step

When you annotate a method with `@Transactional`, Spring doesn't inject the actual bean. It injects a proxy.

```ascii
Client Code --> [ Spring AOP Proxy ] --> [ Transaction Interceptor ] --> [ Actual Target Bean ]
```

1. Client calls `saveUser()`.
2. The Proxy intercepts the call.
3. `TransactionInterceptor` asks `PlatformTransactionManager` to start a transaction.
4. The database connection is bound to the current thread (using `ThreadLocal`).
5. Target method `saveUser()` executes.
6. If success, interceptor commits. If runtime exception, interceptor rolls back.
7. Connection is unbound and returned to the pool.

**Self-Invocation Problem:**
```java
public class UserService {
    public void createUser() {
        // Calling internal method BYPASSES the proxy!
        // No transaction will be started for processInternal().
        processInternal(); 
    }
    
    @Transactional
    public void processInternal() { ... }
}
```

### 3. Auto-configuration Flow

1. `@SpringBootApplication` implies `@EnableAutoConfiguration`.
2. Spring parses `META-INF/spring.factories` (or `AutoConfiguration.imports` in Boot 3).
3. It loads classes like `DataSourceAutoConfiguration`.
4. It evaluates conditions: `@ConditionalOnClass(DataSource.class)`, `@ConditionalOnMissingBean`.
5. If conditions are met, it registers the beans into the `ApplicationContext`.

### 4. Embedded Tomcat Thread Pool Architecture

```ascii
[ OS Network Stack ]
       |
       v (Port 8080)
+-------------------------------------------+
| Acceptor Thread (listens for connections) |
+----------------------+--------------------+
                       | passes SocketChannel
+----------------------v--------------------+
| Poller Threads (NIO Selector)             |
+----------------------+--------------------+
                       | assigns to worker
+----------------------v--------------------+
| Worker Thread Pool (max 200 by default)   |
| [Thread-1] [Thread-2] ... [Thread-200]    |
+----------------------+--------------------+
                       |
               [ Spring MVC DispatcherServlet ]
```
If all 200 threads are busy, Tomcat queues requests (up to `accept-count`, default 100). If the queue fills, connections are refused.

### 5. Spring WebFlux vs Spring MVC

**Spring MVC:** Thread-per-request model. Blocks on I/O (DB, HTTP calls). Simple to reason about but memory-heavy at massive scale.
**Spring WebFlux:** Reactive model (Project Reactor - `Mono`/`Flux`). Event-loop based (Netty). Does not block threads on I/O. Extremely efficient memory usage but hard to debug, trace, and requires reactive DB drivers (R2DBC).
**Note:** With Java 21 Virtual Threads, Spring MVC can handle the same massive scale as WebFlux without the cognitive overhead of reactive programming.

---

## Part 4: Spring Boot + Concurrency in Production

### 1. `@Async` Full Configuration

Without custom configuration, `@Async` uses `SimpleAsyncTaskExecutor`, which creates a *new thread for every task* (terrible for production).

```java
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.scheduling.annotation.EnableAsync;
import org.springframework.scheduling.concurrent.ThreadPoolTaskExecutor;
import java.util.concurrent.Executor;

@Configuration
@EnableAsync
public class AsyncConfig {

    @Bean(name = "taskExecutor")
    public Executor taskExecutor() {
        ThreadPoolTaskExecutor executor = new ThreadPoolTaskExecutor();
        executor.setCorePoolSize(10);
        executor.setMaxPoolSize(50);
        executor.setQueueCapacity(100);
        executor.setThreadNamePrefix("AsyncWorker-");
        
        // Critical: Handle rejections when queue is full!
        executor.setRejectedExecutionHandler(new java.util.concurrent.ThreadPoolExecutor.CallerRunsPolicy());
        
        executor.initialize();
        return executor;
    }
}
```

### 2. `@Scheduled` with Thread Pool

By default, all `@Scheduled` tasks run on a single thread. If one slow cron job runs, it blocks all others!

```java
@Configuration
@EnableScheduling
public class SchedulerConfig implements SchedulingConfigurer {
    @Override
    public void configureTasks(ScheduledTaskRegistrar taskRegistrar) {
        ThreadPoolTaskScheduler scheduler = new ThreadPoolTaskScheduler();
        scheduler.setPoolSize(5); // Now 5 scheduled tasks can run concurrently
        scheduler.setThreadNamePrefix("ScheduledTask-");
        scheduler.initialize();
        taskRegistrar.setTaskScheduler(scheduler);
    }
}
```

### 3. HikariCP Sizing Formula

Formula provided by PostgreSQL experts: `connections = ((core_count * 2) + effective_spindle_count)`.
For a server with 8 cores and 1 SSD (effective spindle count = 1), the optimal pool size is 17.

```properties
spring.datasource.hikari.maximum-pool-size=17
spring.datasource.hikari.minimum-idle=5
spring.datasource.hikari.connection-timeout=30000
spring.datasource.hikari.idle-timeout=600000
```
*Why not 1000?* High concurrent connections cause the database OS to thrash context switching between processes. A small, fast pool processes transactions faster.

### 4. @Transactional Isolation Levels

- **READ_UNCOMMITTED**: Dirty reads allowed (reading data another tx hasn't committed).
- **READ_COMMITTED**: (Postgres/Oracle default) Prevents dirty reads.
- **REPEATABLE_READ**: (MySQL default) Prevents non-repeatable reads (data changing during your tx).
- **SERIALIZABLE**: Prevents phantom reads (new rows being inserted matching your query).

### 5. Optimistic vs Pessimistic Locking in JPA

**Optimistic Locking** (Assumes low collision, uses a version column):
```java
@Entity
public class Account {
    @Id private Long id;
    private BigDecimal balance;
    
    @Version
    private Integer version; // Hibernate auto-increments this
}
```
If two threads read version=1, and try to update, Thread 1 updates version to 2. Thread 2 tries to update where version=1 (fails, throws `OptimisticLockException`).

**Pessimistic Locking** (Assumes high collision, locks row in DB):
```java
public interface AccountRepository extends JpaRepository<Account, Long> {
    @Lock(LockModeType.PESSIMISTIC_WRITE)
    @Query("SELECT a FROM Account a WHERE a.id = :id")
    Optional<Account> findByIdForUpdate(Long id); // Translates to SELECT ... FOR UPDATE
}
```

---

## Part 5: Common Interview Problems (Full Implementations)

### 1. Thread-Safe Singleton (All Patterns)

**Double-Checked Locking:**
```java
public class SingletonDCL {
    // volatile is MANDATORY to prevent partial object exposure due to instruction reordering
    private static volatile SingletonDCL instance;

    private SingletonDCL() {}

    public static SingletonDCL getInstance() {
        if (instance == null) { // First check (no lock)
            synchronized (SingletonDCL.class) {
                if (instance == null) { // Second check (locked)
                    instance = new SingletonDCL();
                }
            }
        }
        return instance;
    }
}
```

**Enum Pattern (Joshua Bloch recommended - safe from reflection/serialization):**
```java
public enum SingletonEnum {
    INSTANCE;
    public void doWork() {}
}
```

**Bill Pugh Initialization-on-Demand Holder Idiom:**
```java
public class SingletonHolder {
    private SingletonHolder() {}

    // Inner class is not loaded until getInstance() is called
    private static class LazyHolder {
        static final SingletonHolder INSTANCE = new SingletonHolder();
    }

    public static SingletonHolder getInstance() {
        return LazyHolder.INSTANCE;
    }
}
```

### 2. Producer-Consumer with BlockingQueue

```java
import java.util.concurrent.*;

public class ProducerConsumer {
    public static void main(String[] args) {
        BlockingQueue<Integer> queue = new LinkedBlockingQueue<>(5); // Max capacity 5

        Runnable producer = () -> {
            try {
                for (int i = 0; i < 20; i++) {
                    System.out.println("Producing: " + i);
                    queue.put(i); // Blocks if queue is full
                    Thread.sleep(100);
                }
            } catch (InterruptedException e) { Thread.currentThread().interrupt(); }
        };

        Runnable consumer = () -> {
            try {
                while (true) {
                    Integer val = queue.take(); // Blocks if queue is empty
                    System.out.println("Consuming: " + val);
                    Thread.sleep(500); // Slower consumer
                }
            } catch (InterruptedException e) { Thread.currentThread().interrupt(); }
        };

        new Thread(producer).start();
        new Thread(consumer).start();
    }
}
```

### 3. Custom Thread Pool Implementation

```java
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

public class SimpleThreadPool {
    private final BlockingQueue<Runnable> taskQueue;
    private final Thread[] workerThreads;
    private volatile boolean isRunning = true;

    public SimpleThreadPool(int numThreads) {
        taskQueue = new LinkedBlockingQueue<>();
        workerThreads = new Thread[numThreads];

        for (int i = 0; i < numThreads; i++) {
            workerThreads[i] = new Thread(new Worker());
            workerThreads[i].start();
        }
    }

    public void execute(Runnable task) {
        if (isRunning) {
            taskQueue.offer(task);
        }
    }

    public void shutdown() {
        isRunning = false;
        for (Thread t : workerThreads) {
            t.interrupt();
        }
    }

    private class Worker implements Runnable {
        public void run() {
            while (isRunning || !taskQueue.isEmpty()) {
                try {
                    Runnable task = taskQueue.take(); // Block until task available
                    task.run();
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                } catch (Exception e) {
                    // Log error, don't let thread die
                    e.printStackTrace();
                }
            }
        }
    }
}
```

### 4. Dining Philosophers (Preventing Deadlock)

```java
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

public class DiningPhilosophers {
    static class Philosopher extends Thread {
        private final Lock leftFork;
        private final Lock rightFork;

        public Philosopher(Lock left, Lock right) {
            this.leftFork = left;
            this.rightFork = right;
        }

        public void run() {
            try {
                while (true) {
                    // Think
                    Thread.sleep(100);
                    // To prevent deadlock, impose an order. Always lock lowest hashcode first.
                    Lock first = leftFork.hashCode() < rightFork.hashCode() ? leftFork : rightFork;
                    Lock second = leftFork.hashCode() < rightFork.hashCode() ? rightFork : leftFork;

                    first.lock();
                    try {
                        second.lock();
                        try {
                            // Eat
                            System.out.println(Thread.currentThread().getName() + " is eating.");
                            Thread.sleep(100);
                        } finally {
                            second.unlock();
                        }
                    } finally {
                        first.unlock();
                    }
                }
            } catch (InterruptedException e) {}
        }
    }
}
```

### 5. Rate Limiter with Semaphore

```java
import java.util.concurrent.Semaphore;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

public class RateLimiter {
    private final Semaphore semaphore;
    private final int maxPermits;

    public RateLimiter(int permitsPerSecond) {
        this.maxPermits = permitsPerSecond;
        this.semaphore = new Semaphore(permitsPerSecond);
        
        // Refill permits every second
        ScheduledExecutorService scheduler = Executors.newScheduledThreadPool(1);
        scheduler.scheduleAtFixedRate(() -> {
            semaphore.drainPermits();
            semaphore.release(maxPermits);
        }, 1, 1, TimeUnit.SECONDS);
    }

    public boolean allowRequest() {
        return semaphore.tryAcquire();
    }
}
```

### 6. Deadlock Example & Detection

```java
public class DeadlockDemo {
    private static final Object lock1 = new Object();
    private static final Object lock2 = new Object();

    public static void main(String[] args) {
        Thread t1 = new Thread(() -> {
            synchronized (lock1) {
                System.out.println("T1 holds lock1");
                try { Thread.sleep(100); } catch (Exception e) {}
                synchronized (lock2) { System.out.println("T1 holds lock2"); }
            }
        });

        Thread t2 = new Thread(() -> {
            synchronized (lock2) {
                System.out.println("T2 holds lock2");
                try { Thread.sleep(100); } catch (Exception e) {}
                synchronized (lock1) { System.out.println("T2 holds lock1"); }
            }
        });

        t1.start(); t2.start();
    }
}
```
**Detection in Production:** Use `jstack <pid>` or JDK Flight Recorder. The output will explicitly state "Found one Java-level deadlock:" and list the blocking threads.

---
*End of Exhaustive Reference. Good luck on the interview!*
