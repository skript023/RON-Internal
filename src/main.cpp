#include "common.hpp"
#include "gui.hpp"
#include "hooking.hpp"
#include "pointers.hpp"
#include "renderer.hpp"
#include "menu_settings.hpp"
#include "benchmark.hpp"
#include "thread_pool.hpp"
#include "task_mgr.hpp"
#include "settings/settings.hpp"
#include "commands/commands.hpp"

#include "events/main_event.hpp"
#include "events/entity_event.hpp"
#include "events/mission_event.hpp"
#include "events/server_event.hpp"
#include "server/server_module.hpp"

#include "discord/discord.hpp"
#include "notification/notification_service.hpp"
#include <process_event/process_event_service.hpp>

// The Astra loader reads "<install folder>\ReadyOrNot Mod". The game runs from ReadyOrNot\Binaries\Win64, so the install
// folder is the first one above it that holds Unreal's Engine folder.
static std::filesystem::path game_install_folder()
{
	wchar_t image[MAX_PATH]{};
	const std::filesystem::path exe(std::wstring(image, GetModuleFileNameW(nullptr, image, MAX_PATH)));
	std::error_code ec;
	for (auto dir = exe.parent_path(); dir.has_relative_path(); dir = dir.parent_path())
	{
		if (std::filesystem::is_directory(dir / "Engine", ec))
			return dir;
	}
	return exe.parent_path();
}

// Earlier builds kept their files in %APPDATA%\ReadyOrNot. The first start from the new folder brings the settings,
// menu settings, saved teleports and window layout along, and leaves the old folder in place.
static bool migrate_legacy_folder(const std::filesystem::path& base_dir)
{
	const char* appdata = std::getenv("appdata");
	std::error_code ec;
	if (!appdata || std::filesystem::exists(base_dir / "settings.json", ec))
		return false;
	const auto legacy_dir = std::filesystem::path(appdata) / FOLDER_NAME;
	if (!std::filesystem::is_directory(legacy_dir, ec))
		return false;

	bool migrated = false;
	for (const char* name : {"settings.json", "menu_settings.json", "teleport_location.json", "imgui.ini"})
		migrated |= std::filesystem::copy_file(legacy_dir / name, base_dir / name, std::filesystem::copy_options::skip_existing, ec);
	return migrated;
}

DWORD APIENTRY main_thread(LPVOID)
{
	using namespace big;

	while (!FindWindow(WINDOW_CLASS, WINDOW_NAME))
		std::this_thread::sleep_for(1s);

	benchmark initialization_benchmark("Initialization");

	const auto base_dir = game_install_folder() / MOD_FOLDER_NAME;

	g_file_manager.init(base_dir);
	const bool migrated = migrate_legacy_folder(base_dir);

	settings::initialize(g_file_manager.get_project_file("./settings.json"));

	g_logger.initialize("Quantum", g_file_manager.get_project_file("./RON.log"), g_settings.debug.external_console);
	if (migrated)
		LOG(INFO) << "Copied the settings from %APPDATA%\\" FOLDER_NAME " to " << base_dir.string() << ".";

	try
	{
		LOG(INFO_RAW) << R"kek(
   ____                    _                   
  / __ \                  | |                  
 | |  | |_   _  __ _ _ __ | |_ _   _ _ __ ___  
 | |  | | | | |/ _` | '_ \| __| | | | '_ ` _ \ 
 | |__| | |_| | (_| | | | | |_| |_| | | | | | |
  \___\_\\__,_|\__,_|_| |_|\__|\__,_|_| |_| |_|
)kek";
		g_settings.load();
		LOG(INFO) << "Settings initialized.";

		auto discord_instance = std::make_unique<discord>();
		LOG(INFO) << "Discord rich presence initialized.";

		auto pointers_instance = std::make_unique<pointers>();
		LOG(INFO) << "Pointers initialized.";

		auto renderer_instance = std::make_unique<renderer>();
		LOG(INFO) << "Renderer initialized.";

		auto thread_pool_instance = std::make_unique<thread_pool>(std::thread::hardware_concurrency() / 2);
		LOG(INFO) << "Thread Pool initialized.";

		auto hooking_instance = std::make_unique<hooking>();
		LOG(INFO) << "Hooking initialized.";

		g_pointers->update();
		LOG(INFO) << "Pointer cached.";
		
		//auto server_instance = std::make_unique<server_module>();
		LOG(INFO) << "Server initialized.";

		// Every task is a coroutine resumed on the game thread, where the engine's objects can be read and called safely.
		task_mgr::add(&main_event::run);
		task_mgr::add(&entity_event::run);
		task_mgr::add(&mission_event::run);

		LOG(INFO) << "Tasks registered.";

		g_hooking->enable();
		LOG(INFO) << "Hooking enabled.";

		initialization_benchmark.get_runtime();
		initialization_benchmark.reset();

		while (g_running)
		{
			g_settings.attempt_save();
			// Saves changed feature states and picks up edits the Astra loader made to settings.json.
			settings::tick();
			discord_instance->update();

			std::this_thread::sleep_for(1s);
		}

		g_hooking->disable();
		LOG(INFO) << "Hooking disabled.";

		std::this_thread::sleep_for(1s);

		// Only once the tick hook is off, so no task is being resumed while it is destroyed.
		task_mgr::clear();
		LOG(INFO) << "Tasks unregistered.";

		//server_instance.reset();
		LOG(INFO) << "Server unregistered.";

		hooking_instance.reset();
		LOG(INFO) << "Hooking uninitialized.";

		g_thread_pool->destroy();
		LOG(INFO) << "Destroyed thread pool.";

		thread_pool_instance.reset();
		LOG(INFO) << "Thread Pool uninitialized.";

		renderer_instance.reset();
		LOG(INFO) << "Renderer uninitialized.";

		pointers_instance.reset();
		LOG(INFO) << "Pointers uninitialized.";

		discord_instance.reset();
		LOG(INFO) << "Discord rich presence uninitialized.";

		g_settings.attempt_save();
		LOG(INFO) << "Settings saved and uninitialized.";
	}
	catch (std::exception const& ex)
	{
		LOG(INFO) << ex.what();
		MessageBoxA(nullptr, ex.what(), nullptr, MB_OK | MB_ICONEXCLAMATION);
	}

	// Unloading while another program's hook still jumps into ours, such as an overlay's on Present, would crash the game.
	if (!detour_base::hooks().empty())
	{
		LOG(WARNING) << "DLL retained because another program still calls into its hooks. Restart the game before injecting again.";
		g_logger.destroy();
		CloseHandle(g_main_thread);
		return 0;
	}

	LOG(INFO) << "Farewell!";
	g_logger.destroy();

	CloseHandle(g_main_thread);
	FreeLibraryAndExitThread(g_hmodule, 0);

	return 0;
}

BOOL APIENTRY DllMain(HMODULE hmod, DWORD reason, PVOID)
{
	using namespace big;

	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hmod);

		g_hmodule = hmod;
		g_main_thread = CreateThread(nullptr, 0, &main_thread, nullptr, 0, &g_main_thread_id);
		break;
	case DLL_PROCESS_DETACH:
		g_running = false;
		break;
	}

	return true;
}
