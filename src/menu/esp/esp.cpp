#include "esp.h"
#include <gui.hpp>
#include "core/submenu.hpp"
#include "utility/player.hpp"
#include <features/features.hpp>
#include "events/entity_event.hpp"

namespace big
{
    void esp::draw_bone(SDK::AReadyOrNotCharacter* actor, SDK::APlayerController* controller, SDK::FString a, SDK::FString b, Color color)
    {
        if (!actor || !controller) return;

        auto a_world = unreal_engine::get_location_bone(actor, a);
        auto b_world = unreal_engine::get_location_bone(actor, b);

        SDK::FVector2D a_screen, b_screen;

        if (!controller->ProjectWorldLocationToScreen(a_world, &a_screen, false))
            return;

        if (!controller->ProjectWorldLocationToScreen(b_world, &b_screen, false))
            return;

        canvas::draw_line(
            a_screen.X,
            a_screen.Y,
            b_screen.X,
            b_screen.Y,
            color,
            1.f
        );
    }
    void esp::draw_skeleton(SDK::AReadyOrNotCharacter* target, SDK::APlayerController* controller, Color const& colour)
    {
        if (!target || !controller) return;

        for (auto& p : torso)
            draw_bone(target, controller, p.first, p.second, colour);

        for (auto& p : left_arm)
            draw_bone(target, controller, p.first, p.second, colour);

        for (auto& p : right_arm)
            draw_bone(target, controller, p.first, p.second, colour);

        for (auto& p : left_leg)
            draw_bone(target, controller, p.first, p.second, colour);

        for (auto& p : right_leg)
            draw_bone(target, controller, p.first, p.second, colour);
    }
    static void draw_box3d(SDK::AReadyOrNotCharacter* target, SDK::APlayerController* controller, Color col)
    {
        if (!target) return;

        SDK::FVector origin, extent;
        target->GetActorBounds(true, &origin, &extent, false);

        SDK::FVector top = origin + SDK::FVector(0, 0, extent.Z);
        SDK::FVector bottom = origin - SDK::FVector(0, 0, extent.Z);

        SDK::FVector2D top2D, bottom2D;

        if (!controller->ProjectWorldLocationToScreen(top, &top2D, false))
            return;

        if (!controller->ProjectWorldLocationToScreen(bottom, &bottom2D, false))
            return;

        float height = fabs(bottom2D.Y - top2D.Y);
        float width = height * 0.45f;

        float x = bottom2D.X - width / 2.f;
        float y = top2D.Y;

        canvas::draw_corner_box(x, y, width, height, 1.5f, col);
    }
    static void draw_aimbot_fov(SDK::APlayerController* controller)
    {
        if (!controller || !controller->PlayerCameraManager)
            return;

        auto fov_cmd = commands::get_command<number_command<float>>("aimbot_fov"_hash);
        if (!fov_cmd) return;

        auto pov = controller->PlayerCameraManager->CameraCachePrivate.POV;

        float aimbot_fov = fov_cmd->get_state(); // degree
        float camera_fov = pov.FOV;              // vertical FOV (UE)

        int screen_w, screen_h;
        controller->GetViewportSize(&screen_w, &screen_h);

        float center_x = screen_w * 0.5f;
        float center_y = screen_h * 0.5f;

        // ================================
        // WORLD ANGLE → SCREEN SPACE
        // ================================
        // Aimbot check:
        // acos(dot(view, target)) < fov
        //
        // Circle harus represent sudut di world space
        // jadi harus di project pakai camera FOV

        float aimbot_rad = aimbot_fov * (M_PI / 180.f);
        float camera_rad = camera_fov * (M_PI / 180.f);

        // pakai SCREEN HEIGHT karena UE pakai VERTICAL FOV
        float radius =
            tan(aimbot_rad * 0.5f) /
            tan(camera_rad * 0.5f) *
            (screen_h * 0.5f);

        // ================================
        // DRAW
        // ================================

        canvas::draw_circle(
            center_x,
            center_y,
            radius,
            Color(255, 255, 255, 180),
            64
        );
    }
    void esp::draw_esp()
	{
        float width = static_cast<float>(g_pointers->m_resolution->x / 2);
        float height = static_cast<float>(g_pointers->m_resolution->y / 2);

        auto controller = unreal_engine::get_player_controller();

        Color red = { 255, 0, 0, 255 };
        Color blue = { 90, 130, 180, 200 };
        Color green = { 90, 160, 120, 200 };
        Color white = { 255, 255, 255, 255 };

        if (!controller)
			return;

        if (!features::_esp_enabled.get_state())
            return;

        const auto view = g_esp_data.view();

        if (!view) return;

        for (const auto& data : *view)
        {
            if (features::_draw_fov.get_state())
                draw_aimbot_fov(controller);

            if (features::_draw_line.get_state())
                canvas::draw_line(width, 0, data.screen.X, data.screen.Y, data.color, 1.f);
            if (features::_draw_skeleton.get_state())
                draw_skeleton(data.actor, controller, white);
            if (features::_draw_box.get_state())
                draw_box3d(data.actor, controller, white);
            if (features::_draw_name.get_state())
            {
                if (data.actor)
                {
                    SDK::FVector2D sceen;
                    auto feet = unreal_engine::get_location_bone(data.actor, L"Root");

                    if (!controller->ProjectWorldLocationToScreen(feet, &sceen, false))
                        continue;

                    canvas::draw_stroke_text(sceen.X, sceen.Y, data.color, data.display_text);
                }
                else
                {
                    canvas::draw_stroke_text(data.screen.X, data.screen.Y, data.color, data.display_text);
                }
            }
        }
	}
}