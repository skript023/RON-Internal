#include "script.hpp"
#include "utility/misc.hpp"
#include "utility/player.hpp"
#include "events/entity_event.hpp"
#include "commands/float_command.hpp"
#include "commands/number_command.hpp"
#include "commands/looped_command.hpp"
#include "process_event/process_event.hpp"
#include "SDK/ReadyOrNot_classes.hpp"
#include "SDK/ReadyOrNot_parameters.hpp"


namespace big
{
	struct recoil_backup
	{
		float RecoilMultiplierPitch;
		float RecoilMultiplierYaw;
		float FirstShotRecoil;
		float RecoilFireStrength;
		float RecoilFireStrengthFirst;
		float RecoilAngleStrength;
		float RecoilRandomness;
		float RecoilFireADSModifier;
		float RecoilAngleADSModifier;
		float RecoilBuildupADSModifier;
		bool bRecoilHasBuildup;
		float RecoilBuildupDampStrength;
		float Wobble;
		float FirstShotResetTime;

		float VelocityRecoilMultiplier;
		float ADSRecoilMultiplier;

		float RecoilReturnRate;
		float RecoilReturnInterpSpeed;
		float RecoilReturnPercentage;
		float RecoilDampStrength;

		bool bCalculateProcRecoil;
		bool bUseFireLoopAnims;
	};

	static std::unordered_map<SDK::ABaseMagazineWeapon*, recoil_backup> g_backup;

	static void backup_weapon(SDK::ABaseMagazineWeapon* wep)
	{
		if (!wep) return;

		// kalau belum pernah dibackup
		if (g_backup.find(wep) == g_backup.end())
		{
			g_backup[wep] = {
				wep->RecoilMultiplierPitch,
				wep->RecoilMultiplierYaw,
				wep->FirstShotRecoil,
				wep->RecoilFireStrength,
				wep->RecoilFireStrengthFirst,
				wep->RecoilAngleStrength,
				wep->RecoilRandomness,
				wep->RecoilFireADSModifier,
				wep->RecoilAngleADSModifier,
				wep->RecoilBuildupADSModifier,
				wep->RecoilHasBuildup,
				wep->RecoilBuildupDampStrength,
				wep->Wobble,
				wep->FirstShotResetTime,

				wep->VelocityRecoilMultiplier,
				wep->ADSRecoilMultiplier,

				wep->RecoilReturnRate,
				wep->RecoilReturnInterpSpeed,
				wep->RecoilReturnPercentage,
				wep->RecoilDampStrength,

				wep->bCalculateProcRecoil,
				wep->bUseFireLoopAnims
			};
		}
	}

	static void apply_no_recoil(SDK::ABaseMagazineWeapon* wep)
	{
		if (!wep) return;

		wep->RecoilMultiplierPitch = 0.f;
		wep->RecoilMultiplierYaw = 0.f;
		wep->FirstShotRecoil = 0.f;
		wep->RecoilFireStrength = 0.f;
		wep->RecoilFireStrengthFirst = 0.f;
		wep->RecoilAngleStrength = 0.f;
		wep->RecoilRandomness = 0.f;
		wep->RecoilFireADSModifier = 0.f;
		wep->RecoilAngleADSModifier = 0.f;
		wep->RecoilBuildupADSModifier = 0.f;
		wep->RecoilHasBuildup = false;
		wep->RecoilBuildupDampStrength = 0.f;
		wep->Wobble = 0.f;
		wep->FirstShotResetTime = 0.f;

		wep->VelocityRecoilMultiplier = 0.f;
		wep->ADSRecoilMultiplier = 0.f;

		wep->RecoilReturnRate = 0.f;
		wep->RecoilReturnInterpSpeed = 0.f;
		wep->RecoilReturnPercentage = 0.f;
		wep->RecoilDampStrength = 0.f;

		wep->bCalculateProcRecoil = false;
		wep->bUseFireLoopAnims = false;
	}

	static void restore_all()
	{
		for (auto& [wep, data] : g_backup)
		{
			if (!wep) continue;

			wep->RecoilMultiplierPitch = data.RecoilMultiplierPitch;
			wep->RecoilMultiplierYaw = data.RecoilMultiplierYaw;
			wep->FirstShotRecoil = data.FirstShotRecoil;
			wep->RecoilFireStrength = data.RecoilFireStrength;
			wep->RecoilFireStrengthFirst = data.RecoilFireStrengthFirst;
			wep->RecoilAngleStrength = data.RecoilAngleStrength;
			wep->RecoilRandomness = data.RecoilRandomness;
			wep->RecoilFireADSModifier = data.RecoilFireADSModifier;
			wep->RecoilAngleADSModifier = data.RecoilAngleADSModifier;
			wep->RecoilBuildupADSModifier = data.RecoilBuildupADSModifier;
			wep->RecoilHasBuildup = data.bRecoilHasBuildup;
			wep->RecoilBuildupDampStrength = data.RecoilBuildupDampStrength;
			wep->Wobble = data.Wobble;
			wep->FirstShotResetTime = data.FirstShotResetTime;

			wep->VelocityRecoilMultiplier = data.VelocityRecoilMultiplier;
			wep->ADSRecoilMultiplier = data.ADSRecoilMultiplier;

			wep->RecoilReturnRate = data.RecoilReturnRate;
			wep->RecoilReturnInterpSpeed = data.RecoilReturnInterpSpeed;
			wep->RecoilReturnPercentage = data.RecoilReturnPercentage;
			wep->RecoilDampStrength = data.RecoilDampStrength;

			wep->bCalculateProcRecoil = data.bCalculateProcRecoil;
			wep->bUseFireLoopAnims = data.bUseFireLoopAnims;
		}

		g_backup.clear();
	}

	class no_recoil : public looped_command
	{
		using looped_command::looped_command;

		void on_enable() override
		{
			if (auto c = static_cast<SDK::APlayerCharacter*>(unreal_engine::get_character()))
			{
				if (auto wep = c->GetEquippedWeapon())
				{
					backup_weapon(wep);
				}
			}
		}

		void on_tick() override
		{
			if (auto c = static_cast<SDK::APlayerCharacter*>(unreal_engine::get_character()))
			{
				if (auto wep = c->GetEquippedWeapon())
				{
					apply_no_recoil(wep);
				}
			}
		}

		void on_disable() override
		{
			restore_all();
		}
	};

	static no_recoil _no_recoil("no_recoil", "No Spread", "");
}