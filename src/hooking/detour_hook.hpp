#pragma once
#include "detour_base.hpp"

namespace ellohim
{
	class detour_hook;
}

namespace big
{
	class detour_hook : public detour_base
	{
	public:
		explicit detour_hook(const std::string_view name, void* target, void* detour);
		~detour_hook() noexcept;

		bool enable();
		bool disable();

		void enable_immediately() const;
		void disable_immediately() const;

		void* get_original_ptr();

		template<typename T>
		T get_original()
		{
			return static_cast<T>(get_original_ptr());
		}
		template<auto T>
		static void add(std::string_view name, void* target)
		{
			detour_base::add<T>(new detour_hook(name, target, T));
		}
		void fix_hook_address();

	private:
		std::unique_ptr<ellohim::detour_hook> m_backend;
	};
}
