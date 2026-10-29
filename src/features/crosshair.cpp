#include "crosshair.hpp"
#include "script.hpp"
#include "utility/player.hpp"
#include "commands/commands.hpp"
#include "commands/looped_command.hpp"
#include "SDK/ReadyOrNot_classes.hpp"
#include "SDK/ReadyOrNot_parameters.hpp"

#include <astra/host/canvas.hpp>

namespace big
{
	struct crosshair_place
	{
		bool shown = false;
		// In the middle of the screen when no gun says otherwise.
		bool centred = true;
		float x = 0.f;
		float y = 0.f;
	};

	// Written on the game thread, drawn on the render thread.
	static std::mutex g_mutex;
	static crosshair_place g_place;

	// Logged from shots in first and third person: a shot leaves the gun's bullet spawn along its barrel, not the
	// camera, so the crosshair goes on the first thing in line with the barrel, up to 100 m, where the camera sees it.
	// Aiming down sights in first person, the sights take its place.
	class crosshair_command : public looped_command
	{
		using looped_command::looped_command;

		void on_tick() override
		{
			crosshair_place place;
			auto c = unreal_engine::get_character();
			auto controller = unreal_engine::get_player_controller();
			auto third_person = commands::get_bool_command("third_person"_hash);
			place.shown = c && controller && !(c->bAiming && !(third_person && third_person->get_state()));

			auto weapon = place.shown ? c->GetEquippedWeapon() : nullptr;
			if (weapon && weapon->BulletSpawn)
			{
				const auto start = weapon->BulletSpawn->K2_GetComponentLocation();
				const auto end = start + weapon->BulletSpawn->GetForwardVector() * 10000.0;
				SDK::AActor* ignored[] = {weapon};
				SDK::FHitResult hit{};
				const bool blocked = SDK::UKismetSystemLibrary::LineTraceSingle(c, start, end, SDK::ETraceTypeQuery::TraceTypeQuery1, false,
				    SDK::TArray<SDK::AActor*>(ignored, 1, 1), SDK::EDrawDebugTrace::None, &hit, true, {}, {}, 0.f);

				SDK::FVector2D screen;
				const SDK::FVector& target = blocked ? static_cast<const SDK::FVector&>(hit.ImpactPoint) : end;
				if (controller->ProjectWorldLocationToScreen(target, &screen, false))
					place = {true, false, static_cast<float>(screen.X), static_cast<float>(screen.Y)};
			}

			std::lock_guard lock(g_mutex);
			g_place = place;
		}

		void on_disable() override
		{
			std::lock_guard lock(g_mutex);
			g_place = {};
		}
	};

	static crosshair_command _crosshair("crosshair", "Crosshair", "A crosshair on what your gun points at, also over your shoulder in Third Person; hidden while you aim down sights in first person");

	void crosshair::draw()
	{
		if (!_crosshair.get_state())
			return;

		crosshair_place place;
		{
			std::lock_guard lock(g_mutex);
			place = g_place;
		}
		if (!place.shown)
			return;

		const auto display = ImGui::GetIO().DisplaySize;
		const float x = place.centred ? display.x * 0.5f : place.x;
		const float y = place.centred ? display.y * 0.5f : place.y;

		// Four short arms around a dot, white over a dark outline so it shows on any background.
		constexpr float gap = 4.f;
		constexpr float length = 7.f;
		static const Color outline = {0, 0, 0, 200};
		static const Color fill = {255, 255, 255, 230};
		for (const auto& [color, thickness] : {std::pair{outline, 3.5f}, std::pair{fill, 1.5f}})
		{
			canvas::draw_line(x - gap - length, y, x - gap, y, color, thickness);
			canvas::draw_line(x + gap, y, x + gap + length, y, color, thickness);
			canvas::draw_line(x, y - gap - length, x, y - gap, color, thickness);
			canvas::draw_line(x, y + gap, x, y + gap + length, color, thickness);
		}
		canvas::draw_circle_filled(x, y, 2.f, outline);
		canvas::draw_circle_filled(x, y, 1.f, fill);
	}

	bool crosshair::position(float& x, float& y)
	{
		if (!_crosshair.get_state())
			return false;

		std::lock_guard lock(g_mutex);
		if (!g_place.shown || g_place.centred)
			return false;
		x = g_place.x;
		y = g_place.y;
		return true;
	}
}
