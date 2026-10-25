#pragma once
#include "class/triple_buffer.hpp"
#include "task.hpp"

namespace big
{
    struct BonePair { int parent; int child; };

    // ========== SUSPECT ==========
    static constexpr std::array<BonePair, 14> suspect_skel_1 = { {
        {50,51},{9,50},{8,9},{7,8},
        {10,11},{11,12},{12,15},
        {62,63},{63,64},{64,67},
        {103,104},{104,107},
        {115,116},{116,119}
    } };

    static constexpr std::array<BonePair, 14> suspect_skel_2 = { {
        {44,45},{5,44},{3,5},{2,3},
        {6,7},{7,8},{8,11},
        {49,50},{50,51},{51,54},
        {111,112},{112,115},
        {99,100},{100,103}
    } };

    // ========== CIVILIAN ==========
    static constexpr std::array<BonePair, 14> civ_skel_1 = { {
        {50,51},{9,50},{7,9},{1,7},
        {10,11},{11,12},{12,15},
        {62,63},{63,64},{64,67},
        {103,104},{104,107},
        {115,116},{116,119}
    } };

    static constexpr std::array<BonePair, 14> civ_skel_2 = { {
        {44,45},{5,44},{4,5},{3,4},
        {6,7},{7,8},{8,11},
        {49,50},{50,51},{51,54},
        {111,112},{112,115},
        {99,100},{100,103}
    } };

    struct bone_line
    {
        SDK::FVector2D a;
        SDK::FVector2D b;
    };

    enum class esp_kind : std::uint8_t
    {
        suspect,
        civilian,
        swat,
        player,
        trap,
        door,
        evidence_weapon,
        evidence,
        reportable,
        collectible
    };

    // One thing the ESP draws this frame, already projected to screen pixels on the game thread.
    struct esp_data
    {
        esp_kind kind;
        SDK::FVector2D screen;
        float distance;

        std::string display_text;
        Color color;

        float health = 0.f;
        float max_health = 0.f;

        std::vector<bone_line> skeleton;

        float box_x = 0.f;
        float box_y = 0.f;
        float box_w = 0.f;
        float box_h = 0.f;

        bool has_skeleton = false;
        bool has_box = false;
        bool has_box_3d = false;
        std::array<SDK::FVector2D, 8> box_3d{};
    };

    // Written by entity_event on the game thread, read by esp::draw_esp on the render thread.
	inline TripleBuffer<esp_data> g_esp_data{128};

	class entity_event
	{
	public:
		static task<void> run();
	};
}
