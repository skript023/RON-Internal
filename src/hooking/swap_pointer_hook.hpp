#pragma once

namespace big
{
    class swap_pointer_hook
    {
	public:
		explicit swap_pointer_hook(const std::string_view name, void** target, void* detour);

		void enable();
		void disable();

		template <typename T>
		T get_original()
		{
			return static_cast<T>(m_original);
		}
	private:
		void** m_target;
		void* m_swap;
		void* m_original{};
    };
}