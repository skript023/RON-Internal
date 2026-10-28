#include "esp.h"
#include <gui.hpp>
#include "core/submenu.hpp"
#include "utility/player.hpp"
#include <features/features.hpp>
#include "events/entity_event.hpp"
#include "features/hit_marker.hpp"

namespace big
{
    // Runs on the render thread: reads only what entity_event projected on the game thread, plus a few plain fields.
    static void draw_aimbot_fov(SDK::APlayerController* controller = unreal_engine::get_player_controller())
    {
        if (!controller || !controller->PlayerCameraManager)
            return;

        auto fov_cmd = commands::get_command<number_command<float>>("aimbot_fov"_hash);
        if (!fov_cmd) return;

        const auto& pov = controller->PlayerCameraManager->CameraCachePrivate.POV;

        float aimbot_fov_deg = fov_cmd->get_state();
        float camera_fov_deg = pov.FOV;
        if (camera_fov_deg <= 1.f)
            return;

        const auto display = ImGui::GetIO().DisplaySize;
        float screen_w = display.x;
        float screen_h = display.y;

        float center_x = screen_w * 0.5f;
        float center_y = screen_h * 0.5f;

        // The aimbot takes targets up to this angle from the centre; from 90 degrees on that is past the screen edges.
        if (aimbot_fov_deg >= 89.f)
            return;

        float aimbot_rad = aimbot_fov_deg * (M_PI / 180.f);
        float camera_rad = camera_fov_deg * (M_PI / 180.f);

        // The camera's FOV is horizontal, so it spans half the screen width either side of the centre.
        float radius =
            tanf(aimbot_rad) /
            tanf(camera_rad * 0.5f) *
            (screen_w * 0.5f);

        canvas::draw_circle(
            center_x,
            center_y,
            radius,
            Color(255, 255, 255, 180),
            64
        );
    }
    static void draw_box_3d(const std::array<SDK::FVector2D, 8>& p, Color col)
    {
        auto draw = ImGui::GetForegroundDrawList();
        auto color = ImGui::ColorConvertFloat4ToU32(ImVec4(
            col.r / 255.f,
            col.g / 255.f,
            col.b / 255.f,
            col.a / 255.f
        ));

        auto line = [&](int a, int b)
            {
                draw->AddLine({ (float)p[a].X, (float)p[a].Y }, { (float)p[b].X, (float)p[b].Y }, color);
            };

        // bawah
        line(0, 1); line(1, 2); line(2, 3); line(3, 0);

        // atas
        line(4, 5); line(5, 6); line(6, 7); line(7, 4);

        // vertikal
        line(0, 4); line(1, 5); line(2, 6); line(3, 7);
    }
    // A vertical bar left of the box: green at full health, turning red as it drops.
    static void draw_health(const esp_data& data)
    {
        if (!data.has_box || data.max_health <= 0.f || data.box_h <= 1.f)
            return;

        const float ratio = std::clamp(data.health / data.max_health, 0.f, 1.f);
        const float filled = data.box_h * ratio;
        const float x = data.box_x - 6.f;

        canvas::draw_line(x, data.box_y, x, data.box_y + data.box_h, { 0, 0, 0, 255 }, 3.f);

        const Color color{
            static_cast<std::uint8_t>((1.f - ratio) * 255),
            static_cast<std::uint8_t>(ratio * 255),
            0,
            255 };
        canvas::draw_line(x, data.box_y + data.box_h - filled, x, data.box_y + data.box_h, color, 2.f);
    }
    // A top-down panel of what entity_event placed around you, your view pointing up. Something more than a floor above
    // or below you is drawn as a ring.
    static void draw_radar()
    {
        bool open = true;
        if (canvas::begin_panel("Radar", &open, {.size = {260.f, 280.f}}))
        {
            const auto corner = ImGui::GetCursorScreenPos();
            const auto size = ImGui::GetContentRegionAvail();
            const float radius = (std::min)(size.x, size.y) * 0.5f - 4.f;
            const ImVec2 centre{corner.x + size.x * 0.5f, corner.y + size.y * 0.5f};
            if (radius > 8.f)
            {
                auto draw = ImGui::GetWindowDrawList();
                const ImU32 grid = IM_COL32(255, 255, 255, 50);
                draw->AddCircle(centre, radius, grid, 48);
                draw->AddCircle(centre, radius * 0.5f, grid, 48);
                draw->AddLine({centre.x, centre.y - radius}, {centre.x, centre.y + radius}, grid);
                draw->AddLine({centre.x - radius, centre.y}, {centre.x + radius, centre.y}, grid);
                draw->AddTriangleFilled({centre.x, centre.y - 7.f}, {centre.x - 5.f, centre.y + 5.f}, {centre.x + 5.f, centre.y + 5.f}, IM_COL32_WHITE);

                const float scale = radius / features::_radar_range.get_state();
                for (const auto& point : g_radar_data.view())
                {
                    const ImVec2 at{centre.x + point.right * scale, centre.y - point.forward * scale};
                    const ImU32 color = IM_COL32(point.color.r, point.color.g, point.color.b, point.color.a);
                    if (point.other_floor)
                        draw->AddCircle(at, 3.5f, color, 12, 1.5f);
                    else
                        draw->AddCircleFilled(at, 3.5f, color);
                }
            }
        }
        canvas::end_panel();

        // The close button switches the feature off, the same as the menu row.
        if (!open)
            features::_radar.set_state(false);
    }
    void esp::draw_esp()
	{
        if (features::_draw_fov.get_state())
            draw_aimbot_fov();
        hit_marker::draw();

        if (!features::_esp_enabled.get_state())
            return;

        if (features::_radar.get_state())
            draw_radar();

        static const Color arc_color = { 255, 220, 0, 255 };
        for (const auto& line : g_grenade_path.view())
        {
            if (line.a.X == line.b.X && line.a.Y == line.b.Y)
                canvas::draw_circle(line.a.X, line.a.Y, 5.f, arc_color, 16);
            else
                canvas::draw_line(line.a.X, line.a.Y, line.b.X, line.b.Y, arc_color, 2.f);
        }

        const auto display = ImGui::GetIO().DisplaySize;
        const float width = display.x * 0.5f;

        static const Color white = { 255, 255, 255, 255 };

        // Holds the buffer for the whole draw, so the game thread cannot recycle it underneath.
        const auto view = g_esp_data.view();

        for (const auto& data : view)
        {
            if (features::_draw_line.get_state())
                canvas::draw_line(width, 0, data.screen.X, data.screen.Y, data.color, 1.f);
            if (features::_draw_skeleton.get_state() && data.has_skeleton)
            {
                for (auto& line : data.skeleton)
                {
                    canvas::draw_line(line.a.X, line.a.Y, line.b.X, line.b.Y, white, 1.f);
                }
                if (data.head_radius > 0.f)
                    canvas::draw_circle(data.head.X, data.head.Y, data.head_radius, white, 16);
            }
            if (features::_draw_corner_box.get_state() && data.has_box)
                canvas::draw_corner_box(data.box_x, data.box_y, data.box_w, data.box_h, 1.5f, data.color);
            if (features::_draw_box.get_state() && data.has_box)
                canvas::draw_box(data.box_x, data.box_y, data.box_w, data.box_h, 1.5f, data.color);
            if (features::_draw_box_3d.get_state() && data.has_box_3d)
                draw_box_3d(data.box_3d, data.color);
            if (features::_draw_health.get_state())
                draw_health(data);
            if (features::_draw_name.get_state())
            {
                canvas::draw_stroke_text(data.screen.X, data.screen.Y, data.color, data.display_text);
            }
        }
	}
}
