#include <atomic>
#include <vector>
#include <cstddef>
#include <utility>

template<typename T>
class RingBuffer {
private:
    size_t capacity_;                 // 实际数组长度 = 用户容量 + 1（浪费1格）
    std::atomic<size_t> read_;        // 读指针（消费者写）
    std::atomic<size_t> write_;       // 写指针（生产者写）
    std::vector<T> data_;

    size_t next(size_t i) const noexcept {
        return (i + 1) % capacity_;
    }

public:
    explicit RingBuffer(size_t capacity) // 单参数构造用explicit 防止隐式类型转换 RingBuffer<int> rb = 10;
        : capacity_(capacity + 1),
          read_(0),
          write_(0),
          data_(capacity + 1) {}

    // 生产者线程调用
    bool push(const T& value) {
        size_t w = write_.load(std::memory_order_relaxed);
        size_t nw = next(w);

        // 看消费者读到哪了（acquire）
        if (nw == read_.load(std::memory_order_acquire)) //  acquire 获取“消费者已释放槽位(read_)”的保证，并可靠看到最新 read_，避免误判 full；同时保证先判断再写入。
            return false; // full

        data_[w] = value;
        // 发布写入完成（release）
        write_.store(nw, std::memory_order_release);
        // 防止消费者看到write_更新的时候data还没写好
        return true;
    }

SpscRingQue
SpscRingQue
SpscRingQue
SpscRingQue
SpscRingQue
SpscRingQue
SpscRingQue
    // 消费者线程调用
    bool pop(T& out) {
        size_t r = read_.load(std::memory_order_relaxed);

        // 看生产者写到哪了（acquire）
        if (r == write_.load(std::memory_order_acquire)) // 配对实现防止write_更新的时候data还没写好
            return false; // empty

        out = std::move(data_[r]);

        // 释放这个槽位给生产者（release）
        read_.store(next(r), std::memory_order_release);
        // release 发布“我已经读/处理完这个槽位，可以复用”；防止生产者过早复用该槽位写入
        return true;
    }

    bool empty() const {
    return read_.load(std::memory_order_acquire) ==
           write_.load(std::memory_order_acquire);
    }

    bool full() const {
    size_t w  = write_.load(std::memory_order_acquire);
    size_t nw = (w + 1) % capacity_;
    return nw == read_.load(std::memory_order_acquire);
    }

    size_t capacity() const { return capacity_ - 1; } // 可用容量
};


// 后续优化 

// 构造在push的时候 析构发生在pop 而不是vector一次分好 一次回收 

// 固定capacity为2的幂，然后位运算比取模快

// write_ 、read_ 加载到不同的cache line，避免频繁将同一块cache_line标记为脏在其他核心做同步


// 用 raw storage（std::byte/aligned_storage + placement new）管理对象生命周期，做到 只在 push 时构造、pop 时析构，避免 vector<T> 预先默认构造 N 个 T（并且支持 T 无默认构造）。

// 若容量固定为 2^k，可用 idx = (idx + 1) & (cap - 1) 替代 % cap，减少取模开销。

// read_ 和 write_ 做 cache line padding（例如 alignas(64) + padding），避免 false sharing，减少缓存一致性抖动。



#include <thread>
#include <atomic>
#include <cassert>
#include <iostream>
#include <chrono>

void test_spsc_correctness() {
    constexpr std::size_t CAP = 1024;
    constexpr std::size_t N   = 5'000'000;

    std::cout << "=== SPSC Test Start ===\n";
    std::cout << "Capacity = " << CAP
              << ", Total Items = " << N << "\n";

    RingBuffer<std::size_t> rb(CAP);
    std::atomic<bool> done{false};

    auto start = std::chrono::steady_clock::now();

    std::thread producer([&]{
        for (std::size_t i = 0; i < N; ) {
            if (rb.push(i)) {
                ++i;
                if (i % 1'000'000 == 0)
                    std::cout << "[Producer] pushed: " << i << "\n";
            } else {
                std::this_thread::yield();
            }
        }
        done.store(true, std::memory_order_release);
        std::cout << "[Producer] finished.\n";
    });

    std::thread consumer([&]{
        std::size_t expected = 0;
        std::size_t v;

        while (!done.load(std::memory_order_acquire) || !rb.empty()) {
            if (rb.pop(v)) {
                if (v != expected) {
                    std::cerr << "Mismatch: got " << v
                              << " expected " << expected << "\n";
                    std::abort();
                }
                ++expected;

                if (expected % 1'000'000 == 0)
                    std::cout << "[Consumer] popped: " << expected << "\n";
            } else {
                std::this_thread::yield();
            }
        }

        std::cout << "[Consumer] finished. Total consumed: "
                  << expected << "\n";

        assert(expected == N);
    });

    producer.join();
    consumer.join();

    auto end = std::chrono::steady_clock::now();
    double sec = std::chrono::duration<double>(end - start).count();

    std::cout << "=== Test Finished Successfully ===\n";
    std::cout << "Time: " << sec << " sec\n";
    std::cout << "Throughput: "
              << (N / sec) / 1e6 << " M ops/sec\n";
}

int main() {
    test_spsc_correctness();
    return 0;
}






#include <atomic>
#include <array> 
/*
    为什么选择std::array作为缓冲区数据结构？
     固定容量 保持队列有界、连续内存、没有扩容带来的风险、RAII生命周期管理
     vector + capacity 可以保证有界，但是vector没有从类型层面禁止扩容、
     裸指针开辟内存空间的话，带来手动管理内存的复杂性和安全性
*/
template <class T,size_t N>
class SpscRing{
    static_assert((N & (N-1)) == 0,"N must be power of two");
public:

    /*ring buffer 本身用 “full->return false” 的无锁实现，保证逻辑简单可靠；队满时的策略不写死在队列里，而是在业务层决定：比如 EEG/眼动这种实时流可选择“丢旧保新”（先 pop 再 push）*/
    bool push(T&& val) {
        const size_t t = tail_.load(std::memory_order_relaxed);
        const size_t next = (t+1) & mask_;
        if(next == head_.load(std::memory_order_acquire)) return false;
        buf_[t] = std::move(v);
        tail.store(next,std::memory_order_release);
        return true;
    }

    bool pop(T& out) {
        const size_t h = head_.load(std::memory_order_relaxed);
        if (h == tail_.load(std::memory_order_acquire)) return false; // empty
        out = std::move(buf_[h]);
        head_.store((h + 1) & mask_, std::memory_order_release);
        return true;
    }

    bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }

private:
    static constexpr size_t mask_ = N - 1;
    alignas(64) std::array<T,N> buf_;
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
};




// SpscRing.hpp
#pragma once
#include <atomic>
#include <array>
#include <cstddef>
#include <utility>

// N 必须是 2 的幂
template <class T, size_t N>
class SpscRing {
    static_assert((N & (N - 1)) == 0, "N must be power of two");

public:
    bool try_push(T&& v) noexcept {
        const size_t t = tail_.load(std::memory_order_relaxed);
        const size_t next = (t + 1) & mask_;
        if (next == head_.load(std::memory_order_acquire)) return false; // full
        buf_[t] = std::move(v);
        tail_.store(next, std::memory_order_release);
        return true;
    }

    bool try_pop(T& out) noexcept {
        const size_t h = head_.load(std::memory_order_relaxed);
        if (h == tail_.load(std::memory_order_acquire)) return false; // empty
        out = std::move(buf_[h]);
        head_.store((h + 1) & mask_, std::memory_order_release);
        return true;
    }

    bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }

private:
    static constexpr size_t mask_ = N - 1;
    alignas(64) std::array<T, N> buf_{};
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
};
