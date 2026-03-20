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

        container_type& back() noexcept
        {
            return *back_;
        }

        void publish() noexcept
        {
            std::lock_guard lock(m_lock);

            // swap pointer
            std::swap(front_, back_);

            // IMPORTANT: back harus jadi buffer baru (biar gak dipakai render)
            back_ = std::make_shared<container_type>();
        }

        container_ptr view()
        {
            std::lock_guard lock(m_lock);
            return front_;
        }

    private:
        container_ptr buffers_[2];
        container_ptr front_;
        container_ptr back_;
        std::mutex m_lock;
    };
}
