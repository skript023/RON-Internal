#include "script.hpp"
#include "utility/player.hpp"
#include "utility/ron.hpp"
#include "events/entity_event.hpp"
#include "commands/float_command.hpp"
#include "commands/number_command.hpp"
#include "commands/looped_command.hpp"
#include "process_event/process_event.hpp"
#include "SDK/ReadyOrNot_parameters.hpp"

namespace big
{
	bool_command _silent_mode("silent_aimbot", "Silent Aimbot", "Silent Aimbot", false);
	float_command _aimbot_pitch("aimbot_pitch", "Aimbot Pitch", "Pitch adjustment for aimbot", -99999.0f, 99999.0f, 0.0f);
	float_command _aimbot_yaw("aimbot_yaw", "Aimbot Yaw", "Yaw adjustment for aimbot", -99999.0f, 99999.0f, 0.0f);
	number_command<int> _aimbot_trigger("aimbot_trigger", "Aimbot Trigger Type", "0=MouseOnly 1=Aiming 2=Hotkey 3=Mouse+Aiming", 0, 3, 0);
	number_command<float> _aimbot_fov("aimbot_fov", "Aimbot FOV", "Max aim angle", 1.f, 180.f, 20.f);
	number_command<float> _aimbot_smooth("aimbot_smooth", "Aimbot Smooth", "Rotation interpolation", 1.f, 100.f, 10.f );
	bool_command _triggerbot("triggerbot", "Triggerbot", "Auto shoot when enemy is in crosshair", false);
	number_command<float> _trigger_fov("trigger_fov", "Trigger FOV", "Shoot angle tolerance", 0.1f, 5.f, 1.0f);

	enum class aimbot_trigger_mode
	{
		lbutton = 0,
		ads,
		hotkey,
		lbutton_and_ads
	};

	static bool key_down(int vk)
	{
		return (GetAsyncKeyState(vk) & 0x8000) != 0;
	}

	// The modes Aimbot Trigger Type lists: the left button, aiming down sights, the trigger key, or the left button
	// while aiming down sights.
	static bool should_trigger_aimbot(SDK::AReadyOrNotCharacter* ch)
	{
		switch ((aimbot_trigger_mode)_aimbot_trigger.get_state())
		{
		case aimbot_trigger_mode::lbutton:
			return key_down(VK_LBUTTON);

		case aimbot_trigger_mode::ads:
			return ch->bAiming;

		case aimbot_trigger_mode::hotkey:
			return key_down(g_settings.aimbot.trigger_key);

		case aimbot_trigger_mode::lbutton_and_ads:
			return key_down(VK_LBUTTON) && ch->bAiming;
		}

		return false;
	}

	static float normalize_angle(float angle)
	{
		while (angle > 180.f)  angle -= 360.f;
		while (angle < -180.f) angle += 360.f;
		return angle;
	}

	static float delta_angle(float current, float target)
	{
		return normalize_angle(target - current);
	}

	static SDK::FRotator get_aim_rotation(SDK::FVector from, SDK::FVector to)
	{
		SDK::FVector dir = (to - from).Normalize();

		float pitch = std::atan2(
			dir.Z,
			std::sqrt(dir.X * dir.X + dir.Y * dir.Y)
		) * 180.0f / M_PI;

		float yaw = std::atan2(dir.Y, dir.X) * 180.0f / M_PI;

		return SDK::FRotator(pitch, yaw, 0.f);
	}

	static SDK::FRotator smooth_rotation(SDK::FRotator current, SDK::FRotator target, float smooth, float delta)
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

	static SDK::FVector get_aim_target(int index, SDK::AReadyOrNotCharacter* target_pawn)
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

		return {};
	}

	struct aim_target
	{
		SDK::FVector location{};
		float angle = std::numeric_limits<float>::max(); // radians from the centre of the view
	};

	// The suspect nearest the centre of the view that can still fight: in sight, and not down, cuffed or surrendered,
	// since shooting a suspect who gave up costs the mission score.
	static aim_target find_target(SDK::APlayerController* controller, const SDK::FMinimalViewInfo& pov)
	{
		aim_target best;
		const SDK::FVector forward = RotationToVector(pov.Rotation);
		// Seen from the camera, as the player sees it; the capsule's centre sits below a waist-high cover the head clears.
		const auto view_point = pov.Location;

		ron::for_each_ai([&](SDK::ACyberneticCharacter* ai) {
			if (!ai->IsA(SDK::ASuspectCharacter::StaticClass()))
				return;

			auto health = ai->CharacterHealth;
			if (!health)
				return;

			const auto status = health->HealthStatus;
			if (status == SDK::EPlayerHealthStatus::HS_Dead || status == SDK::EPlayerHealthStatus::HS_Downed || status == SDK::EPlayerHealthStatus::HS_Arrested || status == SDK::EPlayerHealthStatus::HS_Incapacitated)
				return;

			if (ai->IsSurrendered() || !controller->LineOfSightTo(ai, view_point, false))
				return;

			auto target = get_aim_target(g_settings.aimbot.aimbot_target, ai);
			// The mesh has no such bone; the actor's own location is still on the suspect.
			if (target.IsZero())
				target = ai->K2_GetActorLocation();

			const float dot = std::clamp(static_cast<float>(forward.Dot((target - pov.Location).Normalize())), -1.f, 1.f);
			if (dot <= 0.f)
				return;

			if (const float angle = std::acos(dot); angle < best.angle)
				best = {target, angle};
		});

		return best;
	}

	class aimbot : public looped_command
	{
		using looped_command::looped_command;

		virtual void on_tick() override
		{
			auto ch = unreal_engine::get_character();
			auto controller = unreal_engine::get_player_controller();
			if (!SDK::UWorld::GetWorld() || !ch || !controller || !controller->AcknowledgedPawn || !controller->PlayerCameraManager)
				return;

			const auto& pov = controller->PlayerCameraManager->CameraCachePrivate.POV;
			const auto target = find_target(controller, pov);
			const auto aimbot_target = target.location;
			const auto aimbot_angle = target.angle;

			// convert FOV dari degree ke rad
			float max_fov = _aimbot_fov.get_state() * (M_PI / 180.0f);
			float trigger_fov = _trigger_fov.get_state() * (M_PI / 180.0f);

			// ================= TRIGGERBOT =================
			if (_triggerbot.get_state() && aimbot_angle < trigger_fov)
			{
				static std::chrono::steady_clock::time_point last_shot{};
				const auto time = std::chrono::steady_clock::now();

				if (time - last_shot > 50ms)
				{
					if (auto wep = ch->GetEquippedWeapon())
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
							100.f,
							delta
						);

						wep->OnFire(smoothed, aimbot_target);
					}
					last_shot = time;
				}
			}

			if (!_silent_mode.get_state())
			{
				// hanya aim kalau target masih dalam FOV legit
				if (aimbot_angle < max_fov && should_trigger_aimbot(ch))
				{
					// rotation target dari posisi camera → target bone
					SDK::FRotator target_rot = get_aim_rotation(
						pov.Location,
						aimbot_target
					);

					// delta time dari world supaya smooth tidak FPS dependent
					float delta = SDK::UGameplayStatics::GetWorldDeltaSeconds(
						SDK::UWorld::GetWorld()
					);

					// The step is measured from the camera, which recoil and sway move off the control rotation, and added
					// to the control rotation. Starting from the camera instead added that offset back every frame and
					// pushed the aim off the target while firing or aiming from the hip.
					const SDK::FRotator camera_rot = pov.Rotation;
					const SDK::FRotator step = smooth_rotation(
						camera_rot,
						target_rot,
						_aimbot_smooth.get_state(),
						delta
					);

					SDK::FRotator control_rot = controller->GetControlRotation();
					control_rot.Pitch = normalize_angle(control_rot.Pitch + normalize_angle(step.Pitch - camera_rot.Pitch));
					control_rot.Yaw = normalize_angle(control_rot.Yaw + normalize_angle(step.Yaw - camera_rot.Yaw));

					controller->SetControlRotation(control_rot);
				}
			}
		}
	};

	class silent_aimbot : public process_event
	{
		using process_event::process_event;

		eExecutionStatus on_call(SDK::UObject* _this, SDK::UFunction* function, void* parms) override
		{
			if (!_silent_mode.get_state())
				return eExecutionStatus::EXECUTION_CONTINUE;

			auto gun = static_cast<SDK::ABaseMagazineWeapon*>(_this);
			auto ch = unreal_engine::get_character();
			auto controller = unreal_engine::get_player_controller();
			// Only the player's own shots; everyone else's reach the game untouched.
			if (!ch || gun->Owner != ch || !controller || !controller->AcknowledgedPawn || !controller->PlayerCameraManager)
				return eExecutionStatus::EXECUTION_CONTINUE;

			const auto target = find_target(controller, controller->PlayerCameraManager->CameraCachePrivate.POV);
			if (target.angle >= _aimbot_fov.get_state() * (M_PI / 180.0f))
				return eExecutionStatus::EXECUTION_CONTINUE;

			auto params = static_cast<SDK::Params::BaseMagazineWeapon_Server_OnFire*>(parms);
			params->SpawnLoc = target.location;

			this->intercept(_this, function, params);

			return eExecutionStatus::EXECUTION_INTERCEPTED;
		}
	};

	static aimbot _aimbot("aimbot", "Aimbot", "Aimbot");
	static silent_aimbot _silent_aimbot("Function ReadyOrNot.BaseMagazineWeapon.Server_OnFire");
}
