#pragma once
#include "class/triple_buffer.hpp"
#include "task.hpp"

namespace big
{
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
        SDK::FVector2D head{};
        float head_radius = 0.f; // 0 without a head to draw

        float box_x = 0.f;
        float box_y = 0.f;
        float box_w = 0.f;
        float box_h = 0.f;

        bool has_skeleton = false;
        bool has_box = false;
        bool has_box_3d = false;
        std::array<SDK::FVector2D, 8> box_3d{};
    };

    // Where something the ESP shows lies from you, in metres along your view and to its right.
    struct radar_point
    {
        float forward;
        float right;
        bool other_floor;
        Color color;
    };

    // Written by entity_event on the game thread, read by esp::draw_esp on the render thread.
	inline TripleBuffer<esp_data> g_esp_data{128};
	inline TripleBuffer<radar_point> g_radar_data{128};
	// The held grenade's arc on screen; a line of no length marks where it first hits something.
	inline TripleBuffer<bone_line> g_grenade_path{128};

	class entity_event
	{
	public:
		static task<void> run();
	};
}
