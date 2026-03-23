#include "esp.h"
#include <gui.hpp>
#include "core/submenu.hpp"
#include "utility/player.hpp"
#include <features/features.hpp>
#include "events/entity_event.hpp"

namespace big
{
    static bool is_skeleton_variant_2(SDK::USkeletalMeshComponent* mesh)
    {
        if (!mesh) return false;

        auto name = mesh->GetBoneName(45).ToString();
        return name == "Head";
    }

    static void draw_bone(SDK::AReadyOrNotCharacter* actor, SDK::APlayerController* controller, int a, int b, Color color)
    {
        if (!actor || !controller) return;

        SDK::FVector a_world, b_world;

        if (!unreal_engine::get_bone_world(actor, a, a_world)) return;
        if (!unreal_engine::get_bone_world(actor, b, b_world)) return;

        SDK::FVector2D a_screen, b_screen;

        if (!controller->ProjectWorldLocationToScreen(a_world, &a_screen, false))
            return;

        if (!controller->ProjectWorldLocationToScreen(b_world, &b_screen, false))
            return;

        canvas::draw_line(
            a_screen.X, a_screen.Y,
            b_screen.X, b_screen.Y,
            color, 1.f
        );
    }
    static void draw_skeleton(SDK::AReadyOrNotCharacter* target, SDK::APlayerController* controller, Color const& colour)
    {
        if (!target || !controller) return;

        auto mesh = target->Mesh;
        if (!mesh) return;

        bool variant2 = is_skeleton_variant_2(mesh);

        auto team = target->DefaultTeam;

        const auto& skeleton =
            team == SDK::ETeamType::TT_SUSPECT
            ? (variant2 ? suspect_skel_2 : suspect_skel_1)
            : (variant2 ? civ_skel_2 : civ_skel_1);

        for (const auto& [p, c] : skeleton)
        {
            draw_bone(target, controller, p, c, colour);
        }
    }
    static void draw_box3d(SDK::AReadyOrNotCharacter* target, SDK::APlayerController* controller, Color col)
    {
        if (!target) return;

        SDK::FVector origin, extent;
        target->GetActorBounds(true, &origin, &extent, false);

        SDK::FVector top = unreal_engine::get_location_bone(target, L"Head");
        SDK::FVector bottom = unreal_engine::get_location_bone(target, L"Root");

        SDK::FVector2D top2D, bottom2D;

        top.Z += 15.f;

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

        float aimbot_fov_deg = fov_cmd->get_state(); // degree (ini yang dipakai di acos)
        float camera_fov_deg = pov.FOV;              // vertical FOV UE

        int screen_w, screen_h;
        controller->GetViewportSize(&screen_w, &screen_h);

        float center_x = screen_w * 0.5f;
        float center_y = screen_h * 0.5f;

        // ================================
        // CONVERT KE RADIAN
        // ================================
        float aimbot_rad = aimbot_fov_deg * (M_PI / 180.f);
        float camera_rad = camera_fov_deg * (M_PI / 180.f);

        // ================================
        // CORE FIX
        // ================================
        // Karena:
        // angle = acos(dot)
        // → itu FULL ANGLE dari center
        //
        // Projection ke screen:
        // radius = tan(angle) / tan(FOV/2) * (screen_h/2)

        float radius =
            tanf(aimbot_rad) /
            tanf(camera_rad * 0.5f) *
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
        auto character = unreal_engine::get_character();

        Color red = { 255, 0, 0, 255 };
        Color blue = { 90, 130, 180, 200 };
        Color green = { 90, 160, 120, 200 };
        Color white = { 255, 255, 255, 255 };

        if (!controller || !character)
        {
            g_esp_data.clear();

            return;
        }

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