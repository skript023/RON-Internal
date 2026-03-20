#pragma once
#include "unreal_engine_utility.hpp"

namespace big::player
{
	inline SDK::FVector get_player_coords()
	{
		if (auto character = unreal_engine::get_character())
		{
			if (auto capsule = character->CapsuleComponent)
			{
				return capsule->RelativeLocation;
			}
		}

		return SDK::FVector(0.f, 0.f, 0.f);
	}

	inline SDK::FRotator get_player_rotation()
	{
		if (auto character = unreal_engine::get_character())
		{
			if (auto capsule = character->CapsuleComponent)
			{
				return capsule->RelativeRotation;
			}
		}

		return SDK::FRotator(0.f, 0.f, 0.f);
	}

	inline SDK::FVector get_player_forward()
	{
		if (auto controller = unreal_engine::get_player_controller())
		{
			return controller->GetActorForwardVector();
		}

		return { 0, 0, 0 };
	}

	inline void set_player_coords(SDK::FVector const& location, SDK::FRotator const& rotator)
	{
		if (auto character = unreal_engine::get_character())
		{
			character->K2_TeleportTo(location, rotator);
			//unreal_engine::get_player_controller()->K2_ClientTravel(L"/Game/Aki/Map/AkiWorld_WP", SDK::ETravelType::TRAVEL_Relative, true);
			unreal_engine::get_player_controller()->ClientAddTextureStreamingLoc(location, 100.f, true);
			unreal_engine::get_player_controller()->ClientCommitMapChange();
		}
	}

	inline float* get_fly_speed()
	{
		if (auto chars = unreal_engine::get_character())
		{
			if (auto movement = chars->CharacterMovement)
			{
				return &movement->MaxFlySpeed;
			}
		}

		return nullptr;
	}

	inline float* get_movement_speed()
	{
		if (auto chars = unreal_engine::get_character())
		{
			if (auto movement = chars->CharacterMovement)
			{
				return &movement->MaxWalkSpeed;
			}
		}

		return nullptr;
	}
	
	inline float* get_swim_speed()
	{
		if (auto chars = unreal_engine::get_character())
		{
			if (auto movement = chars->CharacterMovement)
			{
				return &movement->MaxSwimSpeed;
			}
		}

		return nullptr;
	}
	
	inline float* get_max_acceleration()
	{
		if (auto chars = unreal_engine::get_character())
		{
			if (auto movement = chars->CharacterMovement)
			{
				return &movement->MaxAcceleration;
			}
		}

		return nullptr;
	}
	
	inline float* get_max_custom_move_speed()
	{
		if (auto chars = unreal_engine::get_character())
		{
			if (auto movement = chars->CharacterMovement)
			{
				return &movement->MaxCustomMovementSpeed;
			}
		}

		return nullptr;
	}
	
	inline float* get_max_walk_speed()
	{
		if (auto chars = unreal_engine::get_character())
		{
			if (auto movement = chars->CharacterMovement)
			{
				return &movement->MaxWalkSpeedCrouched;
			}
		}

		return nullptr;
	}
	
	inline void set_movement_speed(bool active, float value)
	{
		if (active)
		{
			if (auto chars = unreal_engine::get_character())
			{
				if (auto movement = chars->CharacterMovement)
				{
					movement->MaxWalkSpeed = value;
					movement->MaxWalkSpeedCrouched = value;
					movement->MaxAcceleration = value;	
				}
			}
		}
	}
	
	inline void set_swim_speed(bool active, float value)
	{
		if (active)
		{
			if (auto chars = unreal_engine::get_character())
			{
				if (auto movement = chars->CharacterMovement)
				{
					movement->MaxSwimSpeed = value;
					movement->MaxAcceleration = value;
				}
			}
		}
	}
	
	inline void set_fly_speed(bool active, float value)
	{
		if (active)
		{
			if (auto chars = unreal_engine::get_character())
			{
				if (auto movement = chars->CharacterMovement)
				{
					movement->MaxFlySpeed = value;
					movement->MaxAcceleration = value;
				}
			}
		}
	}
	
	inline float* get_air_control()
	{
		if (auto chars = unreal_engine::get_character())
		{
			if (auto movement = chars->CharacterMovement)
			{
				return &movement->AirControl;
			}
		}

		return nullptr;
	}

	inline void reset_jump()
	{
		/*if (g_settings.self.inf_jump)
		{
			if (auto c = unreal_engine::get_character())
			{
				c->JumpCurrentCount = 0;
				c->Jump();
			}
		}*/
	}

	//inline void attack_max_speed(float max)
	//{
	//	if (auto c = unreal_engine::get_character())
	//	{
	//		if (auto a = c->AttributeSet)
	//		{
	//			a->SetAttackSpeedMax(max);
	//		}
	//	}
	//}

	inline void teleport_forward()
	{
		auto root = unreal_engine::get_root_component();
		if (auto character = unreal_engine::get_character())
		{
			auto pos = get_player_coords();
			auto rot = get_player_rotation();
			auto forward = get_player_forward();

			pos.X += 100.f + forward.X;
			pos.Y += 100.f + forward.Y;
			pos.Z += 100.f + forward.Z;

			character->K2_TeleportTo(pos, rot);
		}
	}

	inline void no_clip(bool activate)
	{
		static bool bActive = false;

		if (activate)
		{
			bActive = true;

			auto root = unreal_engine::get_root_component();
			if (auto self = unreal_engine::get_character())
			{
				self->SetActorEnableCollision(false);// &= ~(0xA);
				if (auto movement = self->CharacterMovement)
				{
					movement->MaxFlySpeed = 7000.f;
					movement->MaxWalkSpeed = 7000.f;
					movement->MaxSwimSpeed = 7000.f;
					movement->MaxAcceleration = 15000.f;
					movement->MaxWalkSpeedCrouched = 7000.f;
					movement->MaxCustomMovementSpeed = 7000.f;
					movement->SetMovementMode(SDK::EMovementMode::MOVE_Flying, 0);

					if (unreal_engine::is_key_pressed(VK_SPACE) || unreal_engine::is_controller_pressed(XINPUT_GAMEPAD_A))
						movement->Velocity.Z += 50.f;
					if (unreal_engine::is_key_pressed(VK_LCONTROL) || unreal_engine::is_controller_pressed(XINPUT_GAMEPAD_X))
						movement->Velocity.Z -= 50.f;
				}
			}
		}
		else
		{
			if (bActive)
			{
				if (auto chars = unreal_engine::get_character())
				{
					chars->SetActorEnableCollision(true); //|= 0xA;
					if (auto movement = chars->CharacterMovement)
					{
						movement->MaxFlySpeed = 415.f;
						movement->MaxWalkSpeed = 415.f;
						movement->MaxSwimSpeed = 300.f;
						movement->MaxAcceleration = 1500.f;
						movement->MaxWalkSpeedCrouched = 415.f;
						movement->MaxCustomMovementSpeed = 600.f;
						movement->SetMovementMode(SDK::EMovementMode::MOVE_Walking, 0);
					}
				}

				bActive = false;
			}
		}
	}
}