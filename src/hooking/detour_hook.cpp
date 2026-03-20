#include "detour_hook.hpp"

#include "memory/handle.hpp"

#include <MinHook.h>

namespace big
{
	detour_hook::detour_hook(const std::string_view name, void* target, void* detour) :
		detour_base(name),
		m_target(target),
		m_detour(detour)
	{
		fix_hook_address();

		if (auto status = MH_CreateHook(m_target, m_detour, &m_original); status == MH_OK)
		{
			//LOG(INFO) << "Created hook '" << m_name << "'.";
		}
		else
		{
			throw std::runtime_error(std::format("Failed to create hook '{}' at 0x{:X} (error: {})", m_name, reinterpret_cast<std::uintptr_t>(m_target), MH_StatusToString(status)));
		}
	}

	detour_hook::~detour_hook() noexcept
	{
		if (m_target)
		{
			MH_RemoveHook(m_target);
		}

		LOG(INFO) << "Removed hook '" << m_name << "'.";
	}

	bool detour_hook::enable()
	{
		if (m_enabled) return true;

		if (auto status = MH_QueueEnableHook(m_target); status != MH_OK)
		{
			throw std::runtime_error(std::format("Failed to enable hook 0x{:X} ({})", reinterpret_cast<std::uintptr_t>(m_target), MH_StatusToString(status)));

			return true;
		}

		return false;
	}

	bool detour_hook::disable()
	{
		if (!m_enabled) return false;

		if (auto status = MH_QueueDisableHook(m_target); status != MH_OK)
		{
			LOG(INFO) << "Failed to disable hook '" << m_name << "'.";

			return true;
		}

		return false;
	}

	void detour_hook::enable_immediately() const
	{
		if (auto status = MH_EnableHook(m_target); status == MH_OK)
		{
			LOG(INFO) << "Enabled hook immediately '" << m_name << "'.";
		}
		else
		{
			throw std::runtime_error(std::format("Failed to enable hook 0x{:X} ({})", reinterpret_cast<std::uintptr_t>(m_target), MH_StatusToString(status)));
		}
	}

	void detour_hook::disable_immediately() const
	{
		if (auto status = MH_DisableHook(m_target); status == MH_OK)
		{
			LOG(INFO) << "Disabled hook immediately '" << m_name << "'.";
		}
		else
		{
			LOG(INFO) << "Failed to disable hook '" << m_name << "'.";
		}
	}

	void* detour_hook::get_original_ptr()
	{
		return m_original;
	}

	DWORD exp_handler(PEXCEPTION_POINTERS exp, const std::string_view name)
	{
		return exp->ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION
			? EXCEPTION_EXECUTE_HANDLER
			: EXCEPTION_CONTINUE_SEARCH;
	}

	void detour_hook::fix_hook_address()
	{
		__try
		{
			auto ptr = memory::handle(m_target);

			while (ptr.as<std::uint8_t&>() == 0xE9)
			{
				ptr = ptr.add(1).rip();
			}

			m_target = ptr.as<void*>();
		}
		__except (exp_handler(GetExceptionInformation(), m_name))
		{
			[this]()
				{
					throw std::runtime_error(std::format("Failed to fix hook address for '{}'", m_name));
				}();
		}
	}
}
