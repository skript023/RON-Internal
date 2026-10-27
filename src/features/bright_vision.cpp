#include "commands/float_command.hpp"
#include "commands/looped_command.hpp"
#include "process_event/process_event.hpp"
#include "utility/unreal_engine_utility.hpp"
#include "utility/weak_object.hpp"
#include "SDK/Engine_classes.hpp"
#include "SDK/Engine_parameters.hpp"

namespace big
{
	static float_command _bright_vision_level("bright_vision_level", "Brightness", "", 0.5f, 5.f, 2.f);

	// The camera modifier this feature added to your camera.
	static weak_object<SDK::UCameraModifier> g_bright_modifier;

	// Your character's own camera does not reach the view, so a camera modifier is added instead: every frame the camera
	// asks each modifier for post-process to lay over the level's, and this one answers with a raised exposure. Only
	// your screen changes.
	class bright_vision : public looped_command
	{
		using looped_command::looped_command;

		void on_tick() override
		{
			auto controller = unreal_engine::get_player_controller();
			auto camera = controller ? controller->PlayerCameraManager : nullptr;
			if (!camera)
				return;

			auto modifier = g_bright_modifier.get();
			if (!modifier || modifier->CameraOwner != camera)
				g_bright_modifier = camera->AddNewCameraModifier(SDK::UCameraModifier::StaticClass());
		}

		void on_disable() override
		{
			if (auto modifier = g_bright_modifier.get(); modifier && modifier->CameraOwner)
				modifier->CameraOwner->RemoveCameraModifier(modifier);
			g_bright_modifier = {};
		}
	};

	static bright_vision _bright_vision("bright_vision", "Bright Vision", "Lights up dark rooms on your screen without night vision");

	class bright_post_process : public process_event
	{
		using process_event::process_event;

		eExecutionStatus on_call(SDK::UObject* _this, SDK::UFunction* function, void* parms) override
		{
			auto modifier = g_bright_modifier.get();
			if (!modifier || _this != modifier)
				return eExecutionStatus::EXECUTION_CONTINUE;

			auto p = static_cast<SDK::Params::CameraModifier_BlueprintModifyPostProcess*>(parms);
			p->PostProcessBlendWeight = 1.f;
			p->PostProcessSettings.bOverride_AutoExposureBias = 1;
			p->PostProcessSettings.AutoExposureBias = _bright_vision_level.get_state();
			return eExecutionStatus::EXECUTION_INTERCEPTED;
		}
	};

	static bright_post_process _bright_post_process("Function Engine.CameraModifier.BlueprintModifyPostProcess");
}
