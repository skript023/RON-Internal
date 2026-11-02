#include "script.hpp"
#include "utility/player.hpp"
#include "utility/weak_object.hpp"
#include "commands/looped_command.hpp"
#include "process_event/process_event.hpp"
#include "SDK/ReadyOrNot_classes.hpp"
#include "SDK/ReadyOrNot_parameters.hpp"

namespace big
{
	static SDK::FName camera_style(const wchar_t* name)
	{
		return SDK::UKismetStringLibrary::Conv_StringToName(name);
	}

	// Where the camera sits from your middle, as in over-the-shoulder shooters: how far behind, how far to the right and
	// how high, from the hip and while aiming down sights.
	struct shoulder_view
	{
		float distance;
		float right;
		float up;
	};

	static constexpr shoulder_view hip_view{150.f, 50.f, 60.f};
	static constexpr shoulder_view aim_view{100.f, 45.f, 65.f};

	// The game's own third person switch is empty in the shipping game, so the engine's free camera style puts the view
	// behind you, turned with where you look; only this feature uses that style, so its distance and offset are left as
	// they are afterwards. The pawn is drawn twice: the body the others see under Mesh, hidden from you, and your first
	// person arms, legs and held item under Mesh1P and MeshBody1P, drawn for you alone. While this is on you see the
	// first and not the second, only on your screen.
	class third_person : public looped_command
	{
		using looped_command::looped_command;

		// The pawn whose camera was switched, so a respawned pawn is switched again.
		weak_object<SDK::APlayerCharacter> m_character;
		// Each part switched, listed once with how it was, so turning off puts it back; parts such as a new held item are
		// switched as they come.
		std::vector<std::pair<weak_object<SDK::UPrimitiveComponent>, bool>> m_switched;
		// How far the camera has moved from the hip view to the aiming view, 0 to 1.
		float m_aim = 0.f;

		void switch_parts(SDK::APlayerCharacter* c, SDK::USceneComponent* part, bool show)
		{
			if (!part)
				return;
			if (part == c->Mesh1P || part == c->MeshBody1P)
				show = false;

			if (part->IsA(SDK::UPrimitiveComponent::StaticClass()))
			{
				auto p = static_cast<SDK::UPrimitiveComponent*>(part);
				if (static_cast<bool>(p->bOwnerNoSee) == show)
				{
					p->SetOwnerNoSee(!show);
					const weak_object<SDK::UPrimitiveComponent> entry(p);
					if (std::ranges::find(m_switched, entry, &std::pair<weak_object<SDK::UPrimitiveComponent>, bool>::first) == m_switched.end())
						m_switched.emplace_back(entry, show);
				}

				// The body moves only the clothes and armor it takes as drawn lately, a flag each part updates from its own
				// tick; parts that never tick, such as the armor, could keep it unset once shown and stand still until a
				// weapon swap.
				if (show && p->IsA(SDK::USkinnedMeshComponent::StaticClass()) && !p->IsComponentTickEnabled())
				{
					auto mesh = static_cast<SDK::USkinnedMeshComponent*>(p);
					if (!mesh->bRecentlyRendered)
						mesh->bRecentlyRendered = 1;
				}
			}

			for (auto child : part->AttachChildren)
				switch_parts(c, child, show);
		}

		static bool on_hands(SDK::APlayerCharacter* c, SDK::USceneComponent* part)
		{
			for (auto at = part; at; at = at->AttachParent)
			{
				if (at == c->Mesh1P || at == c->MeshBody1P)
					return true;
			}
			return false;
		}

		void on_tick() override
		{
			auto controller = unreal_engine::get_player_controller();
			auto c = unreal_engine::get_character();
			if (!c || !controller)
				return;

			if (m_character.get() != c)
			{
				controller->ClientSetCameraMode(camera_style(L"FreeCam"));
				m_character = c;
			}
			switch_parts(c, c->Mesh, true);
			switch_parts(c, c->Mesh1P, false);

			// A part hidden as it hung on your first person hands, such as a gun you drop or a grenade you throw, is shown
			// again once it leaves them.
			std::erase_if(m_switched, [c](const auto& entry) {
				auto p = entry.first.get();
				if (!p)
					return true;
				if (entry.second || on_hands(c, p))
					return false;
				p->SetOwnerNoSee(entry.second);
				return true;
			});

			// Eased over an eighth of a second each way.
			const float step = SDK::UGameplayStatics::GetWorldDeltaSeconds(c) * 8.f;
			m_aim = std::clamp(m_aim + (c->bAiming ? step : -step), 0.f, 1.f);
			if (auto camera = controller->PlayerCameraManager)
			{
				camera->FreeCamDistance = std::lerp(hip_view.distance, aim_view.distance, m_aim);
				camera->FreeCamOffset = SDK::FVector(0.0, std::lerp(hip_view.right, aim_view.right, m_aim), std::lerp(hip_view.up, aim_view.up, m_aim));
			}
		}

		void on_disable() override
		{
			if (auto controller = unreal_engine::get_player_controller())
				controller->ClientSetCameraMode(camera_style(L"Default"));
			m_character = {};
			m_aim = 0.f;

			for (const auto& [part, owner_no_see] : m_switched)
			{
				if (auto p = part.get())
					p->SetOwnerNoSee(owner_no_see);
			}
			m_switched.clear();
		}
	};

	static third_person _third_person("third_person", "Third Person", "Shows you from over your right shoulder, closer while you aim down sights");

	// The body's animation works out in native code whether you look through your own eyes, which a body seen from behind
	// is not; its Blueprint update, which comes after, is where that is turned off again before the pose is made.
	class third_person_anim : public process_event
	{
		using process_event::process_event;

		eExecutionStatus on_call(SDK::UObject* _this, SDK::UFunction*, void*) override
		{
			auto c = unreal_engine::get_character();
			auto controller = unreal_engine::get_player_controller();
			if (_third_person.get_state() && c && controller && c->Mesh && _this == c->Mesh->AnimScriptInstance)
			{
				auto anim = static_cast<SDK::URoNAnimInstance_PlayerTP_V3*>(_this);
				anim->bIsLocallyFirstPerson = false;
				// Your own body is never given where you look up or down, so it gets the camera's pitch.
				const double pitch = controller->ControlRotation.Pitch;
				anim->AimOffsets.Y = pitch > 180.0 ? pitch - 360.0 : pitch;
			}
			return eExecutionStatus::EXECUTION_CONTINUE;
		}
	};

	static third_person_anim _third_person_anim("Function ANIMBP_Player_TP_V4.ANIMBP_Player_TP_V4_C.BlueprintUpdateAnimation");
}
