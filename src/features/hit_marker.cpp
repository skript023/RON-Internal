#include "hit_marker.hpp"
#include "commands/bool_command.hpp"
#include "utility/ron.hpp"

#include <astra/host/canvas.hpp>

namespace big
{
	static bool_command _hit_marker("hit_marker", "Hit Marker", "A mark on your crosshair when your shot hits someone, with the damage; red when it takes them down", false);

	using hit_clock = std::chrono::steady_clock;
	constexpr auto g_mark_time = std::chrono::milliseconds(600);

	struct hit_mark
	{
		hit_clock::time_point time;
		float damage;
		bool down;
	};

	// The recent hits, newest last; written on the game thread, drawn on the render thread.
	static std::mutex g_mutex;
	static std::deque<hit_mark> g_marks;

	// Everyone already marked down, kept on the game thread, since the game reports hits on a body too.
	static std::vector<weak_object<SDK::AReadyOrNotCharacter>> g_down;

	void hit_marker::add(SDK::AReadyOrNotCharacter* victim, float damage)
	{
		if (!_hit_marker.get_state())
			return;

		// The game reports a hit once it has applied it, so whether it took them down is already known. A body, or
		// someone another player took down, gets no mark; when their killer is not known yet, as on a client, it is you.
		const weak_object<SDK::AReadyOrNotCharacter> entry(victim);
		const auto known = std::find(g_down.begin(), g_down.end(), entry);
		const bool down = victim->IsDeadOrUnconscious();
		if (!down)
		{
			if (known != g_down.end())
				g_down.erase(known);
		}
		else
		{
			if (known != g_down.end())
				return;
			std::erase_if(g_down, [](const auto& e) { return !e.get(); });
			g_down.push_back(entry);
			const auto by = victim->KilledBy ? victim->KilledBy : victim->IncapacitatedBy;
			if (by && by != unreal_engine::get_character())
				return;
		}

		const auto now = hit_clock::now();
		std::lock_guard lock(g_mutex);
		while (!g_marks.empty() && (g_marks.size() >= 8 || now - g_marks.front().time > g_mark_time))
			g_marks.pop_front();
		g_marks.push_back({now, damage, down});
	}

	void hit_marker::draw()
	{
		if (!_hit_marker.get_state())
			return;

		std::vector<hit_mark> marks;
		{
			std::lock_guard lock(g_mutex);
			marks.assign(g_marks.begin(), g_marks.end());
		}
		if (marks.empty())
			return;

		const auto now = hit_clock::now();
		const auto display = ImGui::GetIO().DisplaySize;
		const float x = display.x * 0.5f;
		const float y = display.y * 0.5f;

		// How far a mark has faded, from 0 when it lands to 1 when it is gone.
		auto faded = [now](const hit_mark& mark) {
			return std::chrono::duration<float>(now - mark.time) / std::chrono::duration<float>(g_mark_time);
		};
		auto color_of = [](const hit_mark& mark, float fade) {
			const auto alpha = static_cast<int>(255.f * (1.f - fade));
			return mark.down ? Color(255, 60, 60, alpha) : Color(255, 255, 255, alpha);
		};

		// A cross around the centre for the newest hit.
		if (const float fade = faded(marks.back()); fade < 1.f)
		{
			constexpr float gap = 6.f;
			constexpr float length = 10.f;
			const auto color = color_of(marks.back(), fade);
			for (const float dx : {-1.f, 1.f})
			{
				for (const float dy : {-1.f, 1.f})
					canvas::draw_line(x + dx * gap, y + dy * gap, x + dx * (gap + length), y + dy * (gap + length), color, 2.f);
			}
		}

		// Each hit's damage rises from beside the cross as it fades.
		for (const auto& mark : marks)
		{
			if (const float fade = faded(mark); fade < 1.f)
				canvas::draw_stroke_text(x + 22.f, y - 22.f - 30.f * fade, color_of(mark, fade), std::format("{:.0f}", mark.damage));
		}
	}
}
