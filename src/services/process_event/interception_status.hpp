#pragma once

namespace big
{
	enum class eExecutionStatus : uint32_t
	{
		EXECUTION_CONTINUE = 1 << 8,
		EXECUTION_INTERCEPTED = 1 << 12
	};
}