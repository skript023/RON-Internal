#include "detour_base.hpp"

namespace big
{
	detour_base::detour_base(const std::string_view name) :
		m_name(name),
		m_enabled(false)
	{
		m_detour_bases.emplace_back(this);
	}

	std::vector<detour_base*>& detour_base::hooks()
	{
		return m_detour_bases;
	}

	bool detour_base::enable_all()
	{
		bool status = true;
		for (auto detour_base : m_detour_bases)
		{
			status = detour_base->enable() && status;
		}
		return status;
	}

	bool detour_base::disable_all()
	{
		bool status = true;
		for (auto detour_base : m_detour_bases)
		{
			status = detour_base->disable() && status;
		}
		return status;
	}
}