#pragma once

#pragma warning(disable:4369 4129 4067 5045 5264, 4005)

#include <SDKDDKVer.h>
#include <Windows.h>
#include <Xinput.h>
#include <d3d9.h>
#include <d3d11.h>
#include <d3d12.h>
#include "dxgi1_4.h"
#include "Psapi.h"
#include <wrl/client.h>

#include <cinttypes>
#include <cstddef>
#include <cstdint>

#include <chrono>
#include <ctime>

#include <filesystem>
#include <source_location>
#include <fstream>
#include <iostream>
#include <iomanip>

#include <atomic>
#include <mutex>
#include <thread>

#include <memory>
#include <new>

#include <sstream>
#include <string>
#include <string_view>

#include <algorithm>
#include <functional>
#include <utility>

#include <stack>
#include <vector>

#include <typeinfo>
#include <type_traits>

#include <exception>
#include <stdexcept>

#include <any>
#include <optional>
#include <variant>

#include <regex>
#include <tlhelp32.h>
#include <unordered_set>

#include <queue>
#include <future>

#include <nlohmann/json.hpp>

#pragma comment(lib, "Xinput.lib")

#define GAME "ReadyOrNotSteam-Win64-Shipping.exe"
#define GAME_NAME "Ready Or Not"
#define FOLDER_NAME "ReadyOrNot"
#define LOG_NAME "ReadyOrNot.log"
#define LOG_EVENT_NAME "ReadyOrNotEvent.log"

#define WINDOW_CLASS "UnrealWindow"
#define WINDOW_NAME "Ready Or Not  "

#include "menu_settings.hpp"
#include "logger.hpp"
#include "exception_handler.hpp"
#include "utility/joaat.hpp"

#define MOD_NAME "Ellohim Private Menu"

#define PRODUCT_NAME "Ellohim"

#define DISTANCE_SPAWN 10

#define CUSTOM_HUD "Madang Hese Udud Hese Teu Boga Duit Wuhan Kontol"

namespace big
{
	using namespace std::chrono_literals;
	
	template <typename T>
	using comptr = Microsoft::WRL::ComPtr<T>;

	inline HMODULE g_hmodule{};
	inline HANDLE g_main_thread{};
	inline DWORD g_main_thread_id{};
	inline std::atomic_bool g_running{ true };
	inline std::atomic_bool g_should_trigger{ false };
	inline int64_t* g_eval_param = nullptr;
}
