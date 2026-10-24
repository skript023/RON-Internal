#pragma once
#include <array>
#include <mutex>
#include <vector>

namespace big
{
	template<typename T>
	class TripleBuffer
	{
	public:
		using container_type = std::vector<T>;

		// Hold this guard for the entire read; publication cannot recycle its buffer.
		class read_view
		{
			std::unique_lock<std::mutex> m_lock;
			const container_type& m_buffer;

		public:
			explicit read_view(const TripleBuffer& owner) :
			    m_lock(owner.m_mutex),
			    m_buffer(owner.m_buffers[owner.m_front_index])
			{
			}
			bool empty() const noexcept
			{
				return m_buffer.empty();
			}
			auto begin() const noexcept
			{
				return m_buffer.begin();
			}
			auto end() const noexcept
			{
				return m_buffer.end();
			}
		};

		TripleBuffer(size_t reserve_size = 0)
		{
			for (auto& buf : m_buffers)
				buf.reserve(reserve_size);

			m_front_index = 0;
			m_back_index = 1;
			m_spare_index = 2;
		}

		// Writer: isi buffer ini
		container_type& back() noexcept
		{
			return m_buffers[m_back_index];
		}

		// Writer: publish hasil write
		void publish() noexcept
		{
			std::lock_guard lock(m_mutex);
			// rotate buffers:
			// back -> front
			// front -> spare
			// spare -> back

			const int old_front = m_front_index;

			m_front_index = m_back_index;

			m_back_index = m_spare_index;
			m_spare_index = old_front;

			// clear buffer baru untuk write berikutnya
			m_buffers[m_back_index].clear();
		}

		// Reader: retain the guard until rendering finishes.
		read_view view() const
		{
			return read_view(*this);
		}

		void clear_all() noexcept
		{
			// Writer only, with no outstanding back() writes.
			std::lock_guard lock(m_mutex);
			for (auto& buffer : m_buffers)
				container_type{}.swap(buffer);
		}

	private:
		std::array<container_type, 3> m_buffers;

		mutable std::mutex m_mutex;
		int m_front_index;
		int m_back_index;
		int m_spare_index;
	};
}
