#include "common.hpp"
#include "gui.hpp"
#include "hooking.hpp"
#include "pointers.hpp"
#include "renderer.hpp"
#include "menu_settings.hpp"
#include "benchmark.hpp"
#include "thread_pool.hpp"
#include "script_mgr.hpp"
#include "settings/settings.hpp"
#include "commands/commands.hpp"

#include "ui/canvas.hpp"
#include "events/main_event.hpp"
#include "events/entity_event.hpp"
#include "events/server_event.hpp"
#include "server/server_module.hpp"

#include "discord/discord.hpp"
#include "notification/notification_service.hpp"
#include <process_event/process_event_service.hpp>

DWORD APIENTRY main_thread(LPVOID)
{
	using namespace big;

	while (!FindWindow(WINDOW_CLASS, WINDOW_NAME))
		std::this_thread::sleep_for(1s);

	benchmark initialization_benchmark("Initialization");

	std::filesystem::path base_dir = std::getenv("appdata");
	base_dir /= FOLDER_NAME;

	g_file_manager.init(base_dir);

	settings::initialize(g_file_manager.get_project_file("./settings.json"));
	
	g_logger.initialize("Quantum", g_file_manager.get_project_file("./FateTrigger.log"), g_settings.debug.external_console);

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

		auto fiber_pool_instance = std::make_unique<fiber_pool>(10);
		LOG(INFO) << "Fiber pool initialized.";

		auto thread_pool_instance = std::make_unique<thread_pool>(std::thread::hardware_concurrency() / 2);
		LOG(INFO) << "Thread Pool initialized.";

		auto hooking_instance = std::make_unique<hooking>();
		LOG(INFO) << "Hooking initialized.";

		g_pointers->update();
		LOG(INFO) << "Pointer cached.";
		
		//auto server_instance = std::make_unique<server_module>();
		LOG(INFO) << "Server initialized.";

		g_script_mgr.add_script(std::make_unique<script>(&main_event::run));
		/*
		g_script_mgr.add_script(std::make_unique<script>(&entity_event::run));*/

		LOG(INFO) << "Scripts registered.";

		g_hooking->enable();
		LOG(INFO) << "Hooking enabled.";

		initialization_benchmark.get_runtime();
		initialization_benchmark.reset();

		g_thread_pool->queue_job([] {
			TRY_CLAUSE
			{
				entity_event::run();
			} EXCEPT_CLAUSE
			});
		//g_thread_pool->queue_job(&main_event::run);

		while (g_running)
		{
			settings::tick();
			g_settings.attempt_save();
			discord_instance->update();

			std::this_thread::sleep_for(1s);
		}

		g_script_mgr.remove_all_scripts();

		g_hooking->disable();
		LOG(INFO) << "Hooking disabled.";

		std::this_thread::sleep_for(1s);

		//server_instance.reset();
		LOG(INFO) << "Server unregistered.";

		hooking_instance.reset();
		LOG(INFO) << "Hooking uninitialized.";

		fiber_pool_instance.reset();
		LOG(INFO) << "Fiber pool uninitialized.";

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
