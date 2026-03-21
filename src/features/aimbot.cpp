#include "script.hpp"
#include "utility/player.hpp"
#include "events/entity_event.hpp"
#include "commands/float_command.hpp"
#include "commands/number_command.hpp"
#include "commands/looped_command.hpp"
#include "process_event/process_event.hpp"
#include "SDK/ReadyOrNot_parameters.hpp"

namespace big
{
	static bool_command _silent_mode("silent_aimbot", "Silent Aimbot", "Silent Aimbot", false);
	static float_command _aimbot_pitch("aimbot_pitch", "Aimbot Pitch", "Pitch adjustment for aimbot", -99999.0f, 99999.0f, 0.0f);
	static float_command _aimbot_yaw("aimbot_yaw", "Aimbot Yaw", "Yaw adjustment for aimbot", -99999.0f, 99999.0f, 0.0f);
	static number_command<int> _aimbot_trigger("aimbot_trigger", "Aimbot Trigger Type", "0=MouseOnly 1=Aiming 2=Hotkey 3=Mouse+Aiming", 0, 3, 0);
	static number_command<float> _aimbot_fov("aimbot_fov", "Aimbot FOV", "Max aim angle", 1.f, 180.f, 20.f);
	static number_command<float> _aimbot_smooth("aimbot_smooth", "Aimbot Smooth", "Rotation interpolation", 1.f, 100.f, 10.f );
	static bool_command _triggerbot("triggerbot", "Triggerbot", "Auto shoot when enemy is in crosshair", false);
	static number_command<float> _trigger_fov("trigger_fov", "Trigger FOV", "Shoot angle tolerance", 0.1f, 5.f, 1.0f);

	enum class aimbot_trigger_mode
	{
		lbutton = 0,
		ads_only,
		hotkey,
		max
	};
	bool should_trigger_aimbot(SDK::AReadyOrNotCharacter* ch)
	{
		switch ((aimbot_trigger_mode)_aimbot_trigger.get_state())
		{
		case aimbot_trigger_mode::lbutton:
			return GetAsyncKeyState(VK_LBUTTON);

		case aimbot_trigger_mode::ads_only:
			return false;//ch->IsAiming() || ch->IsADS();

		case aimbot_trigger_mode::hotkey:
			return GetAsyncKeyState(g_settings.aimbot.trigger_key);

		case aimbot_trigger_mode::max:
			return false;//GetAsyncKeyState(VK_LBUTTON) && (ch->IsAiming() || ch->IsADS());
		}

		return false;
	}

	float normalize_angle(float angle)
	{
		while (angle > 180.f)  angle -= 360.f;
		while (angle < -180.f) angle += 360.f;
		return angle;
	}

	float delta_angle(float current, float target)
	{
		return normalize_angle(target - current);
	}

	SDK::FRotator get_aim_rotation(SDK::FVector from, SDK::FVector to)
	{
		SDK::FVector dir = (to - from).Normalize();

		float pitch = std::atan2(
			dir.Z,
			std::sqrt(dir.X * dir.X + dir.Y * dir.Y)
		) * 180.0f / M_PI;

		float yaw = std::atan2(dir.Y, dir.X) * 180.0f / M_PI;

		return SDK::FRotator(pitch, yaw, 0.f);
	}

	SDK::FRotator smooth_rotation(SDK::FRotator current, SDK::FRotator target, float smooth, float delta)
	{
		float t = std::clamp(delta * smooth, 0.f, 1.f);

		float pitch_delta = normalize_angle(target.Pitch - current.Pitch);
		float yaw_delta = normalize_angle(target.Yaw - current.Yaw);

		SDK::FRotator result;
		result.Pitch = current.Pitch + pitch_delta * t;
		result.Yaw = current.Yaw + yaw_delta * t;
		result.Roll = 0.f;

		result.Pitch = normalize_angle(result.Pitch);
		result.Yaw = normalize_angle(result.Yaw);

		return result;
	}

	SDK::FVector get_aim_target(int index, SDK::AReadyOrNotCharacter* target_pawn)
	{
		auto& targets = g_settings.aimbot.aimbot_targets;

		if (index < 0 || index >= targets.size())
			return {};

		switch (index)
		{
		case 0: return unreal_engine::get_location_bone(target_pawn, L"Head");
		case 1: return unreal_engine::get_location_bone(target_pawn, L"neck_1");
		case 2: return unreal_engine::get_location_bone(target_pawn, L"spine_1");
		case 3: return unreal_engine::get_location_bone(target_pawn, L"pelvis");
		}
	}

	class aimbot : public looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			if (_silent_mode.get_state())
				return;

			// Get current player info
			auto ch = unreal_engine::get_character();
			auto chara_location = player::get_player_forward();
			auto controller = unreal_engine::get_player_controller();
			if (!controller || !controller->AcknowledgedPawn || !controller->PlayerCameraManager || !ch) return;

			auto own_state = controller->AcknowledgedPawn->PlayerState;
			auto pov = controller->PlayerCameraManager->CameraCachePrivate.POV;

			auto aimbot_angle = std::numeric_limits<float>::max();
			SDK::FVector aimbot_target = {};

			// Get player's forward direction from rotation
			SDK::FVector player_forward = RotationToVector(pov.Rotation);

			if (const auto view = g_esp_data.view(); view)
			{
				for (const auto& data : *view)
				{
					auto pawn = data.actor;

					if (!pawn) continue;

					if (!pawn->IsA(SDK::ASuspectCharacter::StaticClass()))
						continue;

					auto sus = static_cast<SDK::ASuspectCharacter*>(pawn);

					if (!sus || !data.enemy) continue;

					if (!controller->LineOfSightTo(sus, player::get_player_coords(), false))
						continue;

					if (data.status == SDK::EPlayerHealthStatus::HS_Dead || data.status == SDK::EPlayerHealthStatus::HS_Downed || data.status == SDK::EPlayerHealthStatus::HS_Arrested || data.status == SDK::EPlayerHealthStatus::HS_Incapacitated)
						continue;

					auto target = get_aim_target(g_settings.aimbot.aimbot_target, sus);

					if (target.IsZero())
					{
						target = pawn->RootComponent->K2_GetComponentToWorld().Translation;
						LOG(WARNING) << "Failed to get head location for target: " << sus->GetName() << ", using root location instead";
					}

					SDK::FVector direction = (target - pov.Location).Normalize();

					float dot = player_forward.Dot(direction);

					if (dot <= 0.f)
						continue;

					dot = std::clamp(dot, -1.f, 1.f);

					float angle = std::acos(dot);

					if (angle < aimbot_angle)
					{
						aimbot_angle = angle;
						aimbot_target = target;
					}
				}
			}

			// convert FOV dari degree ke rad
			float max_fov = _aimbot_fov.get_state() * (M_PI / 180.0f);
			float trigger_fov = _trigger_fov.get_state() * (M_PI / 180.0f);

			// ================= TRIGGERBOT =================
			if (_triggerbot.get_state() && aimbot_angle < trigger_fov)
			{
				static float last_shot = 0.f;

				float time = SDK::UGameplayStatics::GetRealTimeSeconds(
					SDK::UWorld::GetWorld()
				);

				// delay supaya gak full auto 1000rpm
				if (time - last_shot > 0.05f)
				{
					mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
					script::get_current()->yield(1s);
					mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
					last_shot = time;

					//LOG(INFO) << "Triggerbot: Shot fired at target with angle " << aimbot_angle * (180.0f / M_PI) << " degrees";
				}
			}

			// hanya aim kalau target masih dalam FOV legit
			if (aimbot_angle < max_fov && should_trigger_aimbot(ch))
			{
				SDK::FRotator current_rot = pov.Rotation;

				// rotation target dari posisi camera → target bone
				SDK::FRotator target_rot = get_aim_rotation(
					pov.Location,
					aimbot_target
				);

				// delta time dari world supaya smooth tidak FPS dependent
				float delta = SDK::UGameplayStatics::GetWorldDeltaSeconds(
					SDK::UWorld::GetWorld()
				);

				// interpolation supaya tidak snap
				SDK::FRotator smoothed = smooth_rotation(
					current_rot,
					target_rot,
					_aimbot_smooth.get_state(),
					delta
				);

				// apply hasil smooth rotation
				controller->SetControlRotation(smoothed);
			}

			g_should_trigger = false;
		}
	};

	class silent_aimbot : public process_event
	{
		using process_event::process_event;

		eExecutionStatus on_call(SDK::UObject* _this, SDK::UFunction* function, void* parms) override
		{
			if (!_silent_mode.get_state()) return eExecutionStatus::EXECUTION_CONTINUE;

			SDK::ABaseMagazineWeapon* gun = static_cast<SDK::ABaseMagazineWeapon*>(_this);

			auto player = static_cast<SDK::AActor*>(unreal_engine::get_character());

			if (gun->Owner == player)
			{
				auto params = static_cast<SDK::Params::BaseMagazineWeapon_Server_OnFire*>(parms);

				auto ch = unreal_engine::get_character();
				auto chara_location = player::get_player_forward();
				auto controller = unreal_engine::get_player_controller();
				if (!controller || !controller->AcknowledgedPawn || !controller->PlayerCameraManager || !ch) return eExecutionStatus::EXECUTION_CONTINUE;

				auto own_state = controller->AcknowledgedPawn->PlayerState;
				auto pov = controller->PlayerCameraManager->CameraCachePrivate.POV;

				auto aimbot_angle = std::numeric_limits<float>::max();
				SDK::FVector aimbot_target = {};

				// Get player's forward direction from rotation
				SDK::FVector player_forward = RotationToVector(pov.Rotation);

				if (const auto view = g_esp_data.view(); view)
				{
					for (const auto& data : *view)
					{
						auto pawn = data.actor;

						if (!pawn) continue;

						if (!pawn->IsA(SDK::ASuspectCharacter::StaticClass()))
							continue;

						auto sus = static_cast<SDK::ASuspectCharacter*>(pawn);

						if (!sus || !data.enemy) continue;

						if (!controller->LineOfSightTo(sus, player::get_player_coords(), false))
							continue;

						if (data.status == SDK::EPlayerHealthStatus::HS_Dead || data.status == SDK::EPlayerHealthStatus::HS_Downed || data.status == SDK::EPlayerHealthStatus::HS_Arrested || data.status == SDK::EPlayerHealthStatus::HS_Incapacitated)
							continue;

						auto target = get_aim_target(g_settings.aimbot.aimbot_target, sus);

						if (target.IsZero())
						{
							target = pawn->RootComponent->K2_GetComponentToWorld().Translation;
							LOG(WARNING) << "Failed to get head location for target: " << sus->GetName() << ", using root location instead";
						}

						SDK::FVector direction = (target - pov.Location).Normalize();

						float dot = player_forward.Dot(direction);

						if (dot <= 0.f)
							continue;

						dot = std::clamp(dot, -1.f, 1.f);

						float angle = std::acos(dot);

						if (angle < aimbot_angle)
						{
							aimbot_angle = angle;
							aimbot_target = target;
						}
					}
				}

				float max_fov = _aimbot_fov.get_state() * (M_PI / 180.0f);

				if (aimbot_angle < max_fov)
				{
					params->SpawnLoc = aimbot_target;

					this->intercept(_this, function, params);

					return eExecutionStatus::EXECUTION_INTERCEPTED;
				}

				return eExecutionStatus::EXECUTION_CONTINUE;
			}
		}
	};

	static aimbot _aimbot("aimbot", "Aimbot", "Aimbot");
	static silent_aimbot _silent_aimbot("Function ReadyOrNot.BaseMagazineWeapon.Server_OnFire");
}