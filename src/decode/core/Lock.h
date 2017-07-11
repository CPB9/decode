#pragma once

#include "decode/Config.h"

#include <atomic>
#include <thread>
#include <cstdint>

namespace decode {

template <typename T, typename A>
class TtasSpinLock {
public:
    inline TtasSpinLock()
    {
        _lock.store(0, std::memory_order_relaxed);
    }

    void lock()
    {
        do {
            while (_lock.load(std::memory_order_relaxed)) {
                A();
            }
            T value = 0;
            if (_lock.compare_exchange_weak(value, 1, std::memory_order_acquire)) {
                break;
            } else {
                continue;
            }
        } while (true);
    }

    inline void unlock()
    {
        _lock.store(0, std::memory_order_release);
    }

private:
    std::atomic<T> _lock;
};

struct ThreadYieldAction {
    inline void operator()()
    {
        std::this_thread::yield();
    }
};

using Lock = TtasSpinLock<uint8_t, ThreadYieldAction>;

class LockGuard {
public:
    inline LockGuard(Lock* lock)
        : _lock(lock)
    {
        lock->lock();
    }

    inline ~LockGuard()
    {
        _lock->unlock();
    }

    Lock& operator=(const Lock& other) = delete;
    Lock& operator=(Lock&& other) = delete;

private:
    Lock* _lock;
};
}
