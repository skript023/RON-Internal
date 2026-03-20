#include "recursive_mutex.hpp"

#if defined(_WIN32)

RecursiveMutex::RecursiveMutex() {
    // CRITICAL_SECTION di Windows sudah recursive by default
    ::InitializeCriticalSection(&m_mutex);
}

RecursiveMutex::~RecursiveMutex() {
    ::DeleteCriticalSection(&m_mutex);
}

void RecursiveMutex::lock() {
    ::EnterCriticalSection(&m_mutex);
}

bool RecursiveMutex::try_lock() {
    return ::TryEnterCriticalSection(&m_mutex);
}

void RecursiveMutex::unlock() {
    ::LeaveCriticalSection(&m_mutex);
}

#else // POSIX

RecursiveMutex::RecursiveMutex() {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
}

RecursiveMutex::~RecursiveMutex() {
    pthread_mutex_destroy(&m_mutex);
}

void RecursiveMutex::lock() {
    pthread_mutex_lock(&m_mutex);
}

bool RecursiveMutex::try_lock() {
    return pthread_mutex_trylock(&m_mutex) == 0;
}

void RecursiveMutex::unlock() {
    pthread_mutex_unlock(&m_mutex);
}

#endif
