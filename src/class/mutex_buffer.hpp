#pragma once

#pragma once
#include <vector>
#include <memory>
#include "mutex.hpp"
#include "conditional_variable.hpp"

namespace big
{
    template <typename T>
    class MutexDoubleBuffer
    {
    public:
        using container_type = std::vector<T>;
        using container_ptr = std::shared_ptr<container_type>;

        MutexDoubleBuffer()
        {
            buffers_[0] = std::make_shared<container_type>();
            buffers_[1] = std::make_shared<container_type>();
            front_ = buffers_[0];
            back_ = buffers_[1];
        }

        /**
         * @brief Get reference to back buffer for writing.
         * Must lock externally if multithreaded.
         */
        container_type& back() noexcept
        {
            return *back_;
        }

        /**
         * @brief Publish back buffer safely with mutex.
         */
        void publish() noexcept
        {
            m_lock.lock();

            // swap safely
            auto temp = front_;
            front_ = back_;
            back_ = temp;
            back_->clear();

            m_lock.unlock();
        }

        /**
         * @brief Get a snapshot of current front buffer.
         */
        container_ptr view()
        {
            m_lock.lock();
            auto copy = front_; // shared_ptr copy = atomic refcount safe
            m_lock.unlock();
            return copy;
        }

        /**
         * @brief Clear both buffers safely.
         */
        void clear() noexcept
        {
            m_lock.lock();
            front_->clear();
            back_->clear();
            m_lock.unlock();
        }

    private:
        container_ptr buffers_[2];
        container_ptr front_;
        container_ptr back_;
        Mutex m_lock;
    };
}
