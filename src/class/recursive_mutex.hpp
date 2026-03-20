#pragma once

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

class RecursiveMutex {
public:
    RecursiveMutex();
    ~RecursiveMutex();

    void lock();
    bool try_lock();
    void unlock();

private:
#if defined(_WIN32)
    CRITICAL_SECTION m_mutex;
#else
    pthread_mutex_t m_mutex;
#endif
};
