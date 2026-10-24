#include "detour_hook.hpp"
#include <ellohim/hooking/detour_hook.hpp>
namespace big
{
	detour_hook::detour_hook(std::string_view name, void* target, void* detour) :
	    detour_base(name),
	    m_backend(std::make_unique<ellohim::detour_hook>(name, target, detour))
	{
	}
	detour_hook::~detour_hook() noexcept = default;
	bool detour_hook::enable()
	{
		const auto result = m_backend->enable();
		m_enabled = result;
		return result;
	}
	bool detour_hook::disable()
	{
		if (!m_backend->disable())
			return false;
		m_enabled = false;
		return true;
	}
	void detour_hook::enable_immediately() const
	{
		m_backend->enable_immediately();
	}
	void detour_hook::disable_immediately() const
	{
		m_backend->disable_immediately();
	}
	void* detour_hook::get_original_ptr()
	{
		return m_backend->get_original_ptr();
	}
	void detour_hook::fix_hook_address()
	{
	}
}
