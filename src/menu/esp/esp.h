#pragma once
#include "class/enums.hpp"
#include "SDK/ReadyOrNot_classes.hpp"

namespace big
{
	using Bones = std::pair<const wchar_t*, const wchar_t*>;

    static constexpr std::array<Bones, 3> torso = {
        std::pair{ L"Head",     L"neck_1" },
        std::pair{ L"neck_1",   L"spine_3" },
        std::pair{ L"spine_3",  L"torso_stabilizer" },
    };

    static constexpr std::array<Bones, 4> left_arm = {
        std::pair{ L"spine_3",     L"clavicle_LE" },
        std::pair{ L"clavicle_LE", L"upperarm_LE" },
        std::pair{ L"upperarm_LE", L"lowerarm_LE" },
        // optional:
        std::pair{ L"lowerarm_LE", L"hand_LE" },
    };

    static constexpr std::array<Bones, 5> right_arm = {
        std::pair{ L"spine_3",     L"clavicle_RI" },
        std::pair{ L"clavicle_RI", L"upperarm_RI" },
        std::pair{ L"upperarm_RI", L"lowerarm_RI" },
        // optional:
        std::pair{ L"lowerarm_RI", L"hand_RI" },
    };

    static constexpr std::array<Bones, 4> left_leg = {
        std::pair{ L"pelvis",   L"thigh_LE" },
        std::pair{ L"thigh_LE", L"calf_LE" },
        std::pair{ L"calf_LE",  L"foot_LE" },
        std::pair{ L"foot_LE",  L"ball_LE" },
    };

    static constexpr std::array<Bones, 4> right_leg = {
        std::pair{ L"pelvis",   L"thigh_RI" },
        std::pair{ L"thigh_RI", L"calf_RI" },
        std::pair{ L"calf_RI",  L"foot_RI" },
        std::pair{ L"foot_RI",  L"ball_RI" },
    };

	class esp
	{
	public:
		static void draw_bone(
			SDK::AReadyOrNotCharacter* mesh,
			SDK::APlayerController* controller,
            SDK::FString a,
            SDK::FString b,
			Color color
		);
		static void draw_skeleton(
			SDK::AReadyOrNotCharacter* target,
			SDK::APlayerController* controller,
			Color const& colour
		);
		static void draw_esp();
	};
}