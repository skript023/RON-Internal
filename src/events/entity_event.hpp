#pragma once
#include "class/mutex_buffer.hpp"
#include "SDK/ReadyOrNot_classes.hpp"

namespace big
{
    struct esp_data
    {
        SDK::AReadyOrNotCharacter* actor;
        SDK::FVector location;
        SDK::FVector2D screen;
        SDK::FRotator rotation;
        float distance;
        SDK::EPlayerHealthStatus status;

        std::string display_classname;
        std::string display_text;
        Color color;
        bool enemy;
    };

	inline MutexDoubleBuffer<esp_data> g_esp_data;

	class entity_event
	{
	public:
		static void test();
		static void registration();
		static void run();
	};
}