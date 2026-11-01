#include "host_link.hpp"
#include "script.hpp"
#include "utility/ron.hpp"
#include "utility/weak_object.hpp"
#include "commands/commands.hpp"
#include "commands/bool_command.hpp"
#include "process_event/process_event.hpp"
#include "SDK/ReadyOrNot_classes.hpp"
#include "SDK/ReadyOrNot_parameters.hpp"
#include "SDK/Engine_parameters.hpp"


namespace big
{
	// A client running this mod reaches the host's copy of it over the engine's ServerExecRPC, which a shipping build
	// without the mod drops unseen, also once the host unloads it; Client_SendChatMessage answers that one client, and a
	// message starting with this tag is taken off before it reaches the chat. Tagged chat lines from an older client,
	// which asked over Server_SendChatMessage, are still taken.
	//
	// Requests are run:<name>[:<args>] for a mission action or client action, and set:<name>:<value> for a toggle, with
	// 0 for off. Answers are pong or refused for ping, and ok:, failed:, refused: or unknown: before the request. The host
	// also sends pong or refused by itself to every client that asked once Accept Client Requests changes.
	static constexpr std::wstring_view host_link_tag = L"\x1F" L"astra:";

	static bool_command _accept_client_requests("accept_client_requests", "Accept Client Requests", "Clients running this mod may ask you, the host, to run features for them", false);

	// Mission actions that act on the whole mission, so the host running its own command does what the client asked.
	static constexpr joaat_t host_actions[] = {"auto_arrest"_hash, "report_all"_hash, "collect_evidence"_hash, "surrender_suspects"_hash, "disarm_suspects"_hash,
	    "knockout_suspects"_hash, "unlock_doors"_hash, "open_doors"_hash, "disarm_traps"_hash, "remove_wedges"_hash, "auto_win"_hash};

	// Host-only toggles that act on the whole lobby, so a client's request turns the host's own toggle on or off.
	static constexpr joaat_t world_toggles[] = {"freeze_ai"_hash, "slow_motion"_hash, "no_penalty"_hash, "one_kick_doors"_hash, "surrender_on_yell"_hash};

	struct toggle_entry
	{
		joaat_t hash;
		host_link::apply_t apply;
		host_link::remove_t remove;
		host_link::value_t value;
	};

	struct action_entry
	{
		joaat_t hash;
		const char* label;
		host_link::action_t run;
	};

	// Filled while the features' statics are made, so each list is made on first use.
	static std::vector<toggle_entry>& client_toggles()
	{
		static std::vector<toggle_entry> list;
		return list;
	}

	static std::vector<action_entry>& client_actions()
	{
		static std::vector<action_entry> list;
		return list;
	}

	static const toggle_entry* find_toggle(joaat_t hash)
	{
		auto it = std::ranges::find(client_toggles(), hash, &toggle_entry::hash);
		return it != client_toggles().end() ? &*it : nullptr;
	}

	static const action_entry* find_action(joaat_t hash)
	{
		auto it = std::ranges::find(client_actions(), hash, &action_entry::hash);
		return it != client_actions().end() ? &*it : nullptr;
	}

	static bool is_world_toggle(joaat_t hash)
	{
		return std::ranges::find(world_toggles, hash) != std::end(world_toggles);
	}

	host_link::client_toggle::client_toggle(joaat_t toggle, apply_t apply, remove_t remove, value_t value)
	{
		client_toggles().push_back({toggle, apply, remove, value});
	}

	host_link::client_action::client_action(const char* name, const char* label, action_t run)
	{
		client_actions().push_back({joaat(name), label, run});
	}

	// As a client: the controller the host answered. Each new one, on joining a lobby or a map change, asks the host by
	// itself; g_linked keeps the forwarded toggles on meanwhile, and goes once the host refuses or never answers.
	static weak_object<SDK::APlayerController> g_answered;
	static bool g_linked = false;
	static weak_object<SDK::APlayerController> g_asked;
	static int g_asks = 0;
	static std::chrono::steady_clock::time_point g_asked_at{};
	// As a client: Check Host Mod was pressed, so its answer is shown even when nothing changed.
	static bool g_checked = false;
	// As the host: every client that asked, so it runs this mod, and Accept Client Requests as they last heard it.
	static std::vector<weak_object<SDK::AReadyOrNotPlayerController>> g_modded;
	static bool g_accepting = false;
	// As the host: the start of a long request each client is still sending in pieces.
	static std::vector<std::pair<weak_object<SDK::AReadyOrNotPlayerController>, std::string>> g_pieces;
	// As a client: since when a request has had no answer, so a host that stopped answering, such as one that unloaded
	// the mod, is let go.
	static std::optional<std::chrono::steady_clock::time_point> g_waiting;
	// As a client: the value each toggle was last sent with.
	static std::unordered_map<joaat_t, float> g_sent;

	// As the host: each toggle a client has on, with its value.
	struct client_setting
	{
		weak_object<SDK::APlayerController> controller;
		joaat_t toggle;
		float value;
	};
	static std::vector<client_setting> g_client_settings;

	static SDK::APlayerCharacter* pawn_of(SDK::APlayerController* controller)
	{
		auto pawn = controller ? controller->Character : nullptr;
		return pawn && pawn->IsA(SDK::APlayerCharacter::StaticClass()) ? static_cast<SDK::APlayerCharacter*>(pawn) : nullptr;
	}

	static std::wstring widen(const std::string& text)
	{
		return std::wstring(text.begin(), text.end());
	}

	static std::string narrow(std::wstring_view text)
	{
		std::string result;
		for (const auto c : text)
			result += static_cast<char>(c);
		return result;
	}

	// The name in run:<name>:... or set:<name>:..., after any answer before it.
	static std::string name_in(std::string_view request)
	{
		for (const auto kind : {"run:", "set:"})
		{
			if (const auto at = request.find(kind); at != std::string_view::npos)
			{
				auto name = request.substr(at + 4);
				return std::string(name.substr(0, name.find(':')));
			}
		}
		return std::string(request);
	}

	static std::string label_of(const std::string& name)
	{
		const auto hash = joaat(name);
		if (auto action = find_action(hash))
			return action->label;
		auto command = commands::get_command(hash);
		return command ? command->get_label() : name;
	}

	// The engine drops a client sending ServerExecRPC more than 128 characters, so a longer request, such as Give Item's
	// class and attachment paths, goes in pieces: each but the last starts with more:, and the host joins them, in order
	// since the RPC is reliable.
	static void send(SDK::AReadyOrNotPlayerController* controller, const std::string& request)
	{
		constexpr std::size_t piece = 100;
		for (std::size_t at = 0;; at += piece)
		{
			const bool last = at + piece >= request.size();
			const std::wstring text = std::wstring(host_link_tag) + (last ? L"" : L"more:") + widen(request.substr(at, piece));
			controller->ServerExecRPC(SDK::FString(text.c_str()));
			if (last)
				break;
		}

		// Ping goes unanswered by a host without the mod; anything else is sent only to a host that answered one.
		if (request != "ping" && !g_waiting)
			g_waiting = std::chrono::steady_clock::now();
	}

	static void answer(SDK::AReadyOrNotPlayerController* controller, const std::string& text)
	{
		const std::wstring wide = std::wstring(host_link_tag) + widen(text);
		SDK::FRChatMessage reply{};
		reply.Message = SDK::FString(wide.c_str());
		controller->Client_SendChatMessage(reply);
	}

	// As the host: answers a client that asked, and remembers it for when Accept Client Requests changes.
	static void answer_ping(SDK::AReadyOrNotPlayerController* sender)
	{
		if (std::ranges::none_of(g_modded, [&](const auto& modded) { return modded.get() == sender; }))
			g_modded.push_back(sender);
		answer(sender, _accept_client_requests.get_state() ? "pong" : "refused");
	}

	bool host_link::ready()
	{
		auto controller = unreal_engine::get_player_controller();
		return controller && g_answered.get() == controller;
	}

	void host_link::run(const std::string& name, const std::string& label, const std::string& args)
	{
		auto controller = ron::controller();
		if (!ready() || !controller)
		{
			notification::warning(label, "Only works in single player, as the lobby host, or when the host runs this mod (Online Players > Check Host Mod).");
			return;
		}

		send(controller, args.empty() ? "run:" + name : std::format("run:{}:{}", name, args));
		notification::info(label, "Asked the host.");
	}

	void host_link::fly(SDK::APlayerCharacter* character, float speed)
	{
		auto movement = character->CharacterMovement;
		if (!movement)
			return;

		if (character->GetActorEnableCollision())
			character->SetActorEnableCollision(false);
		if (movement->MovementMode != SDK::EMovementMode::MOVE_Flying)
			movement->SetMovementMode(SDK::EMovementMode::MOVE_Flying, 0);
		movement->MaxFlySpeed = speed * 100.f;
		// Stops as soon as you let go, instead of drifting.
		movement->BrakingDecelerationFlying = speed * 400.f;
	}

	void host_link::land(SDK::APlayerCharacter* character)
	{
		character->SetActorEnableCollision(true);
		if (character->CharacterMovement && character->CharacterMovement->MovementMode == SDK::EMovementMode::MOVE_Flying)
			character->CharacterMovement->SetMovementMode(SDK::EMovementMode::MOVE_Walking, 0);
	}

	std::optional<float> host_link::client_value(SDK::AActor* character, joaat_t toggle)
	{
		if (!character)
			return std::nullopt;

		for (const auto& setting : g_client_settings)
		{
			if (setting.toggle == toggle && pawn_of(setting.controller.get()) == character)
				return setting.value;
		}
		return std::nullopt;
	}

	void host_link::tick()
	{
		if (ron::is_host())
		{
			g_linked = false;

			// Clients that asked hear of the change at once, so they need not ask again.
			std::erase_if(g_modded, [](const auto& modded) { return !modded.get(); });
			if (const bool accepting = _accept_client_requests.get_state(); accepting != g_accepting)
			{
				g_accepting = accepting;
				for (const auto& modded : g_modded)
					answer(modded.get(), accepting ? "pong" : "refused");
			}

			// Turning Accept Client Requests off takes back what clients have on, since their requests to turn it off
			// would be refused from then on.
			if (!g_accepting)
			{
				for (const auto& setting : g_client_settings)
				{
					auto pawn = pawn_of(setting.controller.get());
					auto toggle = find_toggle(setting.toggle);
					if (pawn && toggle && toggle->remove)
						toggle->remove(pawn);
				}
				g_client_settings.clear();
				return;
			}

			// The pawn is looked up every tick, so a client's next pawn, such as in the next mission, gets it too.
			std::erase_if(g_client_settings, [](const client_setting& setting) {
				auto controller = setting.controller.get();
				if (!controller)
					return true;

				auto pawn = pawn_of(controller);
				auto toggle = find_toggle(setting.toggle);
				if (pawn && toggle && toggle->apply)
					toggle->apply(pawn, setting.value);
				return false;
			});
			return;
		}

		auto controller = ron::controller();
		if (!ready() || !controller)
		{
			g_sent.clear();
			g_waiting.reset();

			// A new controller may not reach the host at once, so it asks up to three times, five seconds apart.
			if (controller)
			{
				const auto now = std::chrono::steady_clock::now();
				if (g_asked.get() != controller)
				{
					g_asked = controller;
					g_asks = 0;
				}
				if (g_asks < 3 && now - g_asked_at > std::chrono::seconds(5))
				{
					g_asked_at = now;
					++g_asks;
					send(controller, "ping");
				}
				// Not one answer: this host does not run the mod, so the toggles stop waiting for it.
				else if (g_asks == 3 && now - g_asked_at > std::chrono::seconds(5))
				{
					g_linked = false;
				}
			}
			return;
		}

		// From here on the host is treated as one without the mod, until it answers a ping again.
		if (g_waiting && std::chrono::steady_clock::now() - *g_waiting > std::chrono::seconds(5))
		{
			g_answered = {};
			g_linked = false;
			notification::warning("Check Host Mod", "The host stopped answering, so it may have unloaded the mod; nothing more is sent to it.");
			return;
		}

		auto sync = [&](joaat_t hash, float value) {
			auto it = g_sent.find(hash);
			if ((it == g_sent.end() ? 0.f : it->second) == value)
				return;

			g_sent[hash] = value;
			if (auto command = commands::get_command(hash))
				send(controller, std::format("set:{}:{}", command->get_name(), value));
		};

		for (const auto hash : world_toggles)
		{
			if (auto toggle = commands::get_bool_command(hash))
				sync(hash, toggle->get_state() ? 1.f : 0.f);
		}
		for (const auto& entry : client_toggles())
		{
			if (auto toggle = commands::get_bool_command(entry.hash))
				sync(entry.hash, toggle->get_state() ? (entry.value ? entry.value() : 1.f) : 0.f);
		}
	}

	// Also while a new controller asks the host again, so a map change does not turn the toggles off.
	bool host_link::forwards(joaat_t toggle)
	{
		return (ready() || g_linked) && (is_world_toggle(toggle) || find_toggle(toggle));
	}

	// Asks the host whether it runs this mod and takes requests; the answer comes back as a notification.
	class host_link_ping : public command
	{
		using command::command;

		void on_call() override
		{
			if (ron::is_host())
			{
				notification::info(get_label(), "You are the host.");
				return;
			}

			if (auto controller = ron::controller())
			{
				g_checked = true;
				send(controller, "ping");
				notification::info(get_label(), "Asked the host. No answer means it does not run this mod.");
			}
		}
	};

	static host_link_ping _host_link_ping("host_link_ping", "Check Host Mod", "Asks the host whether it runs this mod and accepts requests");

	enum class reply
	{
		ok,
		failed,
		unknown
	};

	// Runs what a client asked for, as the host: mission actions, client actions, world toggles, and client toggles.
	static reply run_request(SDK::AReadyOrNotPlayerController* sender, const std::string& request, const std::string& who)
	{
		if (request.starts_with("run:"))
		{
			const auto rest = request.substr(4);
			const auto colon = rest.find(':');
			const auto name = rest.substr(0, colon);
			const auto hash = joaat(name);
			notification::info("Client Request", std::format("{} asked for {}.", who, label_of(name)));

			if (auto action = find_action(hash))
				return action->run(pawn_of(sender), colon == std::string::npos ? std::string() : rest.substr(colon + 1)) ? reply::ok : reply::failed;

			auto command = std::ranges::find(host_actions, hash) != std::end(host_actions) ? commands::get_command(hash) : nullptr;
			if (!command)
				return reply::unknown;

			command->call();
			return reply::ok;
		}

		if (request.starts_with("set:"))
		{
			const auto rest = request.substr(4);
			const auto colon = rest.rfind(':');
			if (colon == std::string::npos)
				return reply::unknown;

			const auto name = rest.substr(0, colon);
			const auto hash = joaat(name);
			const float value = std::strtof(rest.substr(colon + 1).c_str(), nullptr);
			const bool on = value != 0.f;

			if (is_world_toggle(hash))
			{
				auto toggle = commands::get_bool_command(hash);
				if (!toggle)
					return reply::unknown;
				toggle->set_state(on);
			}
			else if (auto toggle = find_toggle(hash))
			{
				auto it = std::ranges::find_if(g_client_settings, [&](const client_setting& setting) {
					return setting.toggle == hash && setting.controller.get() == sender;
				});
				if (on && it != g_client_settings.end())
				{
					it->value = value;
				}
				else if (on)
				{
					g_client_settings.push_back({sender, hash, value});
				}
				else
				{
					if (it != g_client_settings.end())
						g_client_settings.erase(it);
					if (auto pawn = pawn_of(sender); pawn && toggle->remove)
						toggle->remove(pawn);
				}
			}
			else
			{
				return reply::unknown;
			}

			notification::info("Client Request", std::format("{} turned {} {}.", who, on ? "on" : "off", label_of(name)));
			return reply::ok;
		}

		return reply::unknown;
	}

	// As the host: answers what a client asked.
	static void handle(SDK::AReadyOrNotPlayerController* sender, const std::string& request)
	{
		if (request == "ping")
		{
			answer_ping(sender);
			return;
		}

		if (!_accept_client_requests.get_state())
		{
			answer(sender, "refused:" + request);
			return;
		}

		const auto who = sender->PlayerState ? sender->PlayerState->PlayerNamePrivate.ToString() : std::string("A client");
		switch (run_request(sender, request, who))
		{
		case reply::ok: answer(sender, "ok:" + request); break;
		case reply::failed: answer(sender, "failed:" + request); break;
		case reply::unknown: answer(sender, "unknown:" + request); break;
		}
	}

	// An older client still asks over chat. A client's own call passes through here on its way out, so only the host acts
	// on it.
	class host_link_request : public process_event
	{
		using process_event::process_event;

		eExecutionStatus on_call(SDK::UObject* _this, SDK::UFunction* function, void* parms) override
		{
			const auto& message = static_cast<SDK::Params::ReadyOrNotPlayerController_Server_SendChatMessage*>(parms)->ChatMessage;
			const auto text = message.Message.ToWString();
			if (!text.starts_with(host_link_tag) || !ron::is_host())
				return eExecutionStatus::EXECUTION_CONTINUE;

			// Taken off the chat whether it is answered or not.
			if (_this != unreal_engine::get_player_controller())
				handle(static_cast<SDK::AReadyOrNotPlayerController*>(_this), narrow(std::wstring_view(text).substr(host_link_tag.size())));
			return eExecutionStatus::EXECUTION_INTERCEPTED;
		}
	};

	static host_link_request _host_link_request("Function ReadyOrNot.ReadyOrNotPlayerController.Server_SendChatMessage");

	// A client's request on its way in, as the host; on the client this is its own call on its way out.
	class host_link_exec_request : public process_event
	{
		using process_event::process_event;

		eExecutionStatus on_call(SDK::UObject* _this, SDK::UFunction* function, void* parms) override
		{
			const auto text = static_cast<SDK::Params::PlayerController_ServerExecRPC*>(parms)->Msg.ToWString();
			if (!text.starts_with(host_link_tag) || !ron::is_host() || _this == unreal_engine::get_player_controller() ||
			    !_this->IsA(SDK::AReadyOrNotPlayerController::StaticClass()))
				return eExecutionStatus::EXECUTION_CONTINUE;

			auto sender = static_cast<SDK::AReadyOrNotPlayerController*>(_this);
			auto request = narrow(std::wstring_view(text).substr(host_link_tag.size()));
			std::erase_if(g_pieces, [](const auto& entry) { return !entry.first.get(); });
			auto it = std::ranges::find_if(g_pieces, [&](const auto& entry) { return entry.first.get() == sender; });
			if (request.starts_with("more:"))
			{
				if (it == g_pieces.end())
					g_pieces.push_back({sender, request.substr(5)});
				// Far longer than any request, so a client that never ends one is let go.
				else if (it->second.size() > 4096)
					g_pieces.erase(it);
				else
					it->second += request.substr(5);
				return eExecutionStatus::EXECUTION_INTERCEPTED;
			}

			if (it != g_pieces.end())
			{
				request = it->second + request;
				g_pieces.erase(it);
			}
			handle(sender, request);
			return eExecutionStatus::EXECUTION_INTERCEPTED;
		}
	};

	static host_link_exec_request _host_link_exec_request("Function Engine.PlayerController.ServerExecRPC");

	// The host's answer. On the host this is the call on its way out to a client, so only this game's own controller
	// takes it.
	class host_link_answer : public process_event
	{
		using process_event::process_event;

		eExecutionStatus on_call(SDK::UObject* _this, SDK::UFunction* function, void* parms) override
		{
			const auto& message = static_cast<SDK::Params::ReadyOrNotPlayerController_Client_SendChatMessage*>(parms)->ChatMessage;
			const auto text = message.Message.ToWString();
			const bool local = _this == unreal_engine::get_player_controller();
			if (!text.starts_with(host_link_tag) || !local)
				return eExecutionStatus::EXECUTION_CONTINUE;

			const auto reply = narrow(std::wstring_view(text).substr(host_link_tag.size()));
			const auto label = label_of(name_in(reply));
			g_waiting.reset();
			// Shown when the link comes or goes, or for Check Host Mod; a map change asking again stays quiet.
			if (reply == "pong")
			{
				if (!g_linked || g_checked)
					notification::success("Check Host Mod", "The host runs this mod and accepts requests.");
				g_answered = static_cast<SDK::APlayerController*>(_this);
				g_linked = true;
				g_checked = false;
			}
			else if (reply == "refused")
			{
				if (g_linked || g_checked)
					notification::warning("Check Host Mod", "The host runs this mod but has Accept Client Requests off.");
				g_answered = {};
				g_linked = false;
				g_checked = false;
				// The host says so by itself once it turns them on.
				g_asks = 3;
			}
			else if (reply.starts_with("ok:run:"))
			{
				notification::success(label, "The host did it.");
			}
			else if (reply.starts_with("ok:set:"))
			{
				notification::success(label, reply.ends_with(":0") ? "Turned off by the host." : "Turned on by the host.");
			}
			else if (reply.starts_with("failed:"))
			{
				notification::warning(label, "The host tried, but it did not work there.");
			}
			else if (reply.starts_with("refused:"))
			{
				// The host turned Accept Client Requests off since; the forwarded toggles are turned off here again.
				g_answered = {};
				g_linked = false;
				notification::warning(label, "The host refused. Accept Client Requests is off there.");
			}
			else if (reply.starts_with("unknown:"))
			{
				notification::warning(label, "The host's copy of the mod does not take this request; it may be an older build.");
			}
			return eExecutionStatus::EXECUTION_INTERCEPTED;
		}
	};

	static host_link_answer _host_link_answer("Function ReadyOrNot.ReadyOrNotPlayerController.Client_SendChatMessage");
}
