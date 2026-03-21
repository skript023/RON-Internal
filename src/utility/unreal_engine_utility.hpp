#pragma once
#include <pointers.hpp>
#include "class/enums.hpp"
#include "SDK/ReadyOrNot_classes.hpp"

namespace big::unreal_engine
{
	inline IDXGISwapChain3* get_swapchain()
	{
		if (auto root = *g_pointers->m_renderer)
		{
			if (auto ctx = root->m_game_instance)
			{
				if (auto subsystem = ctx->m_renderer_subsystem)
				{
					if (auto rendering_ctx = subsystem->m_renderer_context)
					{
						if (auto swapchain_wrapper = rendering_ctx->m_swapchain_wrapper)
						{
							LOG(INFO) << "Swapchain Address " << swapchain_wrapper->m_swapchain;

							return swapchain_wrapper->m_swapchain;
						}
					}
				}
			}
		}

		return nullptr;
	}
	
	inline void* get_swapchain_method(int index)
	{
		if (auto root = *g_pointers->m_renderer)
		{
			if (auto ctx = root->m_game_instance)
			{
				if (auto subsystem = ctx->m_renderer_subsystem)
				{
					if (auto rendering_ctx = subsystem->m_renderer_context)
					{
						if (auto swapchain_wrapper = rendering_ctx->m_swapchain_wrapper)
						{
							if (auto swapchain = swapchain_wrapper->m_swapchain)
							{
								auto vtable = *reinterpret_cast<void***>(swapchain);
								return vtable[index];
							}
						}
					}
				}
			}
		}

		return nullptr;
	}

	inline SDK::UObject* get_first_uobject()
	{
		const auto count = SDK::UObject::GObjects->Num();

		for (int i = 0; i < count; ++i)
		{
			auto obj = SDK::UObject::GObjects->GetByIndex(i);
			if (obj->IsDefaultObject())
				return obj;
		}

		return nullptr;
	}

	inline ID3D12CommandQueue* get_command_queue()
	{
		if (auto root = *g_pointers->m_renderer)
		{
			if (auto ctx = root->m_game_instance)
			{
				if (auto subsystem = ctx->m_renderer_subsystem)
				{
					if (auto rendering_ctx = subsystem->m_renderer_context)
					{
						if (auto swapchain_wrapper = rendering_ctx->m_swapchain_wrapper)
						{
							if (auto command_queue_wrapper = swapchain_wrapper->m_command_queue_wrapper)
							{
								LOG(INFO) << "Command Queue Address " << command_queue_wrapper->m_command_queue;

								return command_queue_wrapper->m_command_queue;
							}
						}
					}
				}
			}
		}

		return nullptr;
	}

	inline SDK::ULocalPlayer* get_local_player()
	{
		if (auto engine = g_pointers->m_engine)
		{
			if (auto instance = engine->GameInstance)
			{
				if (auto local_player = instance->LocalPlayers[0])
				{
					return local_player;
				}
			}
		}

		return nullptr;
	}
	
	inline SDK::UGameInstance* get_game_instance()
	{
		if (auto engine = g_pointers->m_engine)
		{
			if (auto instance = engine->GameInstance)
			{
				return instance;
			}
		}

		return nullptr;
	}

	inline SDK::APlayerController* get_player_controller()
	{
		if (auto engine = g_pointers->m_engine)
		{
			if (auto ins = engine->GameInstance)
			{
				if (auto LocalPlayer = ins->LocalPlayers[0])
				{
					if (auto c = LocalPlayer->PlayerController)
					{
						return c;
					}
				}
			}
		}

		return nullptr;
	}

	inline SDK::ULocalPlayer* get_player(int idx)
	{
		if (auto engine = g_pointers->m_engine)
		{
			if (auto ins = engine->GameInstance)
			{
				if (auto LocalPlayer = ins->LocalPlayers[idx])
				{
					return LocalPlayer;
				}
			}
		}

		return nullptr;
	}

	inline SDK::APlayerState* get_player_state(int idx)
	{
		if (auto world = SDK::UWorld::GetWorld())
		{
			if (auto gs = world->GameState)
			{
				if (auto LocalPlayer = gs->PlayerArray[idx])
				{
					return LocalPlayer;
				}
			}
		}

		return nullptr;
	}

	inline std::optional<std::reference_wrapper<SDK::TArray<SDK::APlayerState*>>> get_player_state_list()
	{
		if (auto world = SDK::UWorld::GetWorld())
		{
			if (auto gs = world->GameState)
			{
				return std::ref(gs->PlayerArray);
			}
		}

		return std::nullopt;
	}

	inline SDK::AReadyOrNotCharacter* get_character()
	{
		if (auto player = unreal_engine::get_local_player())
		{
			if (auto pcontroller = player->PlayerController)
			{
				if (auto character = pcontroller->Character)
				{
					return static_cast<SDK::AReadyOrNotCharacter*>(character);
				}
			}
		}

		return nullptr;
	}

	inline SDK::USceneComponent* get_root_component()
	{
		if (auto player = unreal_engine::get_local_player())
		{
			if (auto pcontroller = player->PlayerController)
			{
				if (auto character = pcontroller->Character)
				{
					return character->RootComponent;
				}
			}
		}

		return nullptr;
	}

	inline bool is_valid_actor(void* actor)
	{
		return actor && (uintptr_t)actor > 0x10000;
	}

	inline SDK::FVector get_location_bone(SDK::AReadyOrNotCharacter* character, SDK::FString const& contain)
	{
		if (!character)
			return { 0.f, 0.f, 0.f };

		if (!is_valid_actor(character))
			return { 0.f, 0.f, 0.f };

		auto mesh = character->Mesh;
		if (!mesh || !is_valid_actor(mesh))
			return { 0.f, 0.f, 0.f };

		auto name = SDK::UKismetStringLibrary::Conv_StringToName(contain);

		int index = mesh->GetBoneIndex(name);
		if (index < 0)
			return { 0.f, 0.f, 0.f }; // ✅ penting

		auto bone_name = mesh->GetBoneName(index);

		return mesh->GetSocketLocation(bone_name);
	}

	inline SDK::FVector get_location_bone(SDK::ACharacter* character, EBonesIndex idx)
	{
		if (auto mesh = character->Mesh)
		{
			return mesh->GetSocketLocation(mesh->GetBoneName((UC::int32)idx));
		}
		return { 0.f, 0.f, 0.f };
	}

	inline void enable_input()
	{
		if (auto c = get_player_controller())
		{
			c->EnableInput(c);
		}
	}

	inline void disable_input()
	{
		if (auto c = get_player_controller())
		{
			c->DisableInput(c);
		}
	}

	inline void get_entity()
	{
		if (auto world = *g_pointers->m_world)
		{
			for (auto level : world->Levels)
			{
				if (!level)
					continue;

				for (auto actors : level->Actors)
				{
					if (!actors) continue;

					LOG(INFO) << "Actor name " << actors->GetName();

					if (auto root = actors->RootComponent)
					{

					}
				}
			}
		}
	}

	inline bool is_key_pressed(std::uint16_t key)
	{
		if (GetForegroundWindow() == g_pointers->m_hwnd)
		{
			if (GetAsyncKeyState(key) & 0x8000)
			{
				return true;
			}
		}

		return false;
	}

	inline bool is_controller_pressed(std::uint16_t button)
	{
		XINPUT_STATE state;
		// Zero out the state structure
		ZeroMemory(&state, sizeof(XINPUT_STATE));

		// Get the state of the controller (controller 0)
		if (XInputGetState(0, &state) == ERROR_SUCCESS)
		{
			// Check if the specific button is pressed
			return (state.Gamepad.wButtons & button) != 0;
		}

		// Controller is not connected
		return false;
	}
}