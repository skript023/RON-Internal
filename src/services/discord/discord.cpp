#pragma warning(disable:4005)
#include "discord.hpp"
#include "discord_rpc.h"

static int64_t eptime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

namespace big
{
	discord::discord()
	{
		DiscordEventHandlers handle;
		memset(&handle, 0, sizeof(handle));
		Discord_Initialize("1293554122439786600", &handle, 1, NULL); //Your Api Key
	}
	discord::~discord() noexcept
	{
		Discord_Shutdown();
	}
	void discord::update()
	{
		DiscordRichPresence discord_rpc;
		auto state = std::format("{} With Quantum", GAME_NAME);
		auto text = std::format("{} With Quantum", GAME_NAME);

		memset(&discord_rpc, 0, sizeof(discord_rpc));
		discord_rpc.state = state.c_str();
		discord_rpc.details = "Quantum Cheat Private Version";
		discord_rpc.startTimestamp = eptime;
		discord_rpc.largeImageKey = "icon";
		discord_rpc.largeImageText = text.c_str();
		discord_rpc.smallImageKey = "icon";

		Discord_UpdatePresence(&discord_rpc);
	}
}