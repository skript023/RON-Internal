#include "achievement_event.hpp"
#include "script.hpp"
#include "task_mgr.hpp"
#include "utility/ron.hpp"

#include <astra/host/notification.hpp>

namespace big
{
	namespace
	{
		using clock = std::chrono::steady_clock;

		// What the render thread may read. Everything else here belongs to the game thread.
		struct shared_state
		{
			std::mutex mutex;
			std::vector<steam_achievement> list;
		};

		shared_state g_shared;

		std::atomic<clock::time_point> g_seen{};

		// Steam's own achievement interface, taken from the Steamworks library the game already loaded. The game's own
		// per-achievement calls do nothing, so this is the only way to unlock one by name.
		namespace steam
		{
			using interface_t = void*;

			template <typename Fn>
			Fn export_of(const char* name)
			{
				auto module = GetModuleHandleA("steam_api64.dll");
				return module ? reinterpret_cast<Fn>(GetProcAddress(module, name)) : nullptr;
			}

			interface_t user_stats()
			{
				static interface_t cached = nullptr;
				static bool tried = false;
				if (tried)
					return cached;
				tried = true;

				using factory_t = interface_t (*)();
				if (auto factory = export_of<factory_t>("SteamAPI_SteamUserStats_v012"))
					cached = factory();
				return cached;
			}

			void request_stats(interface_t stats)
			{
				static auto call = export_of<bool (*)(interface_t)>("SteamAPI_ISteamUserStats_RequestCurrentStats");
				if (call)
					call(stats);
			}

			std::uint32_t count(interface_t stats)
			{
				static auto call = export_of<std::uint32_t (*)(interface_t)>("SteamAPI_ISteamUserStats_GetNumAchievements");
				return call ? call(stats) : 0;
			}

			const char* name(interface_t stats, std::uint32_t index)
			{
				static auto call = export_of<const char* (*)(interface_t, std::uint32_t)>("SteamAPI_ISteamUserStats_GetAchievementName");
				return call ? call(stats, index) : nullptr;
			}

			// "name" and "desc" are the two keys Steam's own client shows for an achievement.
			const char* attribute(interface_t stats, const char* api_name, const char* key)
			{
				static auto call =
				    export_of<const char* (*)(interface_t, const char*, const char*)>("SteamAPI_ISteamUserStats_GetAchievementDisplayAttribute");
				return call ? call(stats, api_name, key) : nullptr;
			}

			bool unlocked(interface_t stats, const char* api_name)
			{
				static auto call = export_of<bool (*)(interface_t, const char*, bool*)>("SteamAPI_ISteamUserStats_GetAchievement");
				if (!call)
					return false;

				bool achieved = false;
				call(stats, api_name, &achieved);
				return achieved;
			}

			bool mark(interface_t stats, const char* api_name)
			{
				static auto call = export_of<bool (*)(interface_t, const char*)>("SteamAPI_ISteamUserStats_SetAchievement");
				return call ? call(stats, api_name) : false;
			}

			bool store(interface_t stats)
			{
				static auto call = export_of<bool (*)(interface_t)>("SteamAPI_ISteamUserStats_StoreStats");
				return call ? call(stats) : false;
			}
		}

		void read()
		{
			std::vector<steam_achievement> list;

			if (auto stats = steam::user_stats())
			{
				// Steam wants its stats asked for once before the achievements read back anything.
				static auto asked = false;
				if (!asked)
				{
					asked = true;
					steam::request_stats(stats);
				}

				const auto total = steam::count(stats);
				for (std::uint32_t i = 0; i < total; ++i)
				{
					const auto api = steam::name(stats, i);
					if (!api || !*api)
						continue;

					steam_achievement entry{};
					entry.api_name = api;
					if (const auto display = steam::attribute(stats, api, "name"); display && *display)
						entry.display = display;
					else
						entry.display = api;
					entry.unlocked = steam::unlocked(stats, api);
					list.push_back(std::move(entry));
				}
			}

			std::lock_guard lock(g_shared.mutex);
			g_shared.list = std::move(list);
		}

		void tick()
		{
			// Only while the menu is showing the list.
			static auto last = clock::time_point{};
			if (clock::now() - g_seen.load() > 1s || clock::now() - last < 500ms)
				return;
			last = clock::now();
			read();
		}

		// A fault reading the list skips this read instead of ending the worker.
		void tick_guarded()
		{
			TRY_CLAUSE
			{
				tick();
			}
			EXCEPT_CLAUSE
		}

		void apply_unlock_steam(const std::string& api_name)
		{
			auto stats = steam::user_stats();
			if (!stats)
			{
				notification::warning("Achievements", "Steam's achievement interface is not there.");
				return;
			}

			steam::request_stats(stats);

			// SetAchievement marks it and StoreStats hands it to Steam; a store is refused while one is still running,
			// so it is tried once more before giving up.
			const auto marked = steam::mark(stats, api_name.c_str());
			auto stored = steam::store(stats);
			if (!stored)
				stored = steam::store(stats);

			LOG(INFO) << "Achievement: steam " << api_name << ", marked " << marked << ", stored " << stored;

			if (marked && stored)
				notification::success("Achievements", std::format("{} unlocked in Steam.", api_name));
			else
				notification::warning("Achievements", std::format("Steam did not take {} (marked {}, stored {}).", api_name, marked, stored));
		}

		void apply_unlock_all()
		{
			// The game's own call covers what it knows about, and leaves the rest; those are finished off through Steam,
			// with one store at the end for all of them.
			if (auto controller = ron::controller())
				controller->UnlockAllAchievements();

			auto stats = steam::user_stats();
			if (!stats)
			{
				LOG(INFO) << "Achievement: unlock all asked for (no steam interface, game side only)";
				return;
			}

			auto marked = 0;
			auto refused = 0;
			const auto total = steam::count(stats);
			for (std::uint32_t i = 0; i < total; ++i)
			{
				const auto api = steam::name(stats, i);
				if (!api || !*api || steam::unlocked(stats, api))
					continue;

				if (steam::mark(stats, api))
					++marked;
				else
					++refused;
			}

			// A store is refused while one is still running, so it is tried once more before giving up.
			auto stored = steam::store(stats);
			if (!stored)
				stored = steam::store(stats);

			LOG(INFO) << "Achievement: unlock all asked for, " << marked << " marked in steam, " << refused << " refused, stored " << stored;
		}
	}

	task<void> achievement_event::run()
	{
		LOG(INFO) << "Achievement Event Registered";

		while (g_running)
		{
			tick_guarded();
			co_await this_task::wait();
		}

		co_return;
	}

	std::vector<steam_achievement> achievement_event::all()
	{
		g_seen = clock::now();
		std::lock_guard lock(g_shared.mutex);
		return g_shared.list;
	}

	void achievement_event::unlock_steam(std::string api_name)
	{
		async_run([api_name = std::move(api_name)] {
			apply_unlock_steam(api_name);
		});
	}

	void achievement_event::unlock_all()
	{
		async_run([] {
			apply_unlock_all();
			notification::success("Achievements", "Asked the game to unlock every achievement.");
		});
	}

}
