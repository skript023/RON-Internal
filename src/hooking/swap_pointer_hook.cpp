#include "swap_pointer_hook.hpp"

namespace big
{
	swap_pointer_hook::swap_pointer_hook(const std::string_view name, void** target, void* swap):
		m_target(target),
		m_swap(swap)
	{

	}
	void swap_pointer_hook::enable()
	{
		m_original = *m_target;
		*m_target = m_swap;
	}
	void swap_pointer_hook::disable()
	{
		*m_target = m_original;
	}
}