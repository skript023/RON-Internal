#pragma once

namespace big
{
	enum class eIntegrationStatus
	{
		CONNECTED = 1 << 4,
		DISCONNECTED = 1 << 8,
		UNKNOWN = 1 << 16
	};
}