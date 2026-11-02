#include "visual_filter.hpp"
#include "script.hpp"
#include "task_mgr.hpp"
#include "utility/unreal_engine_utility.hpp"
#include "utility/weak_object.hpp"
#include "SDK/ReadyOrNot_classes.hpp"

#include <astra/host/notification.hpp>

namespace big
{
	struct filter_material
	{
		std::string name;
		weak_object<SDK::UMaterialInterface> material;
	};

	// Written on the game thread, read by the menu on the render thread.
	static std::mutex g_mutex;
	static std::vector<filter_material> g_filters;
	static std::string g_on;
	static std::atomic_bool g_asked{false};

	// Follows the instances up by their fields, so reading the list calls nothing in the engine per material.
	static SDK::UMaterial* base_of(SDK::UMaterialInterface* material)
	{
		for (int depth = 0; material && depth < 8; ++depth)
		{
			if (material->IsA(SDK::UMaterial::StaticClass()))
				return static_cast<SDK::UMaterial*>(material);
			if (!material->IsA(SDK::UMaterialInstance::StaticClass()))
				return nullptr;
			material = static_cast<SDK::UMaterialInstance*>(material)->Parent;
		}
		return nullptr;
	}

	static void read_filters()
	{
		std::vector<filter_material> found;
		for (int i = 0; i < SDK::UObject::GObjects->Num(); ++i)
		{
			auto object = SDK::UObject::GObjects->GetByIndex(i);
			// The game's running copies of its own effects are not filters of their own.
			if (!object || !object->IsA(SDK::UMaterialInterface::StaticClass()) || object->IsA(SDK::UMaterialInstanceDynamic::StaticClass()))
				continue;

			auto name = object->GetName();
			auto material = static_cast<SDK::UMaterialInterface*>(object);
			auto base = name.starts_with("Default__") ? nullptr : base_of(material);
			if (base && base->MaterialDomain == SDK::EMaterialDomain::MD_PostProcess)
				found.push_back({std::move(name), material});
		}
		std::ranges::sort(found, {}, &filter_material::name);

		std::lock_guard lock(g_mutex);
		g_filters = std::move(found);
	}

	// On your character's own post process component, which the game plays its screen effects on, so only your screen
	// changes. The component has no call to remove a material, so weight 0 is off.
	static bool apply(SDK::UMaterialInterface* material, bool on)
	{
		auto c = unreal_engine::get_character();
		auto screen = c ? c->PlayerPostProcessingComp : nullptr;
		if (!screen)
		{
			notification::warning("Visual Filter", "Be in a mission first.");
			return false;
		}

		SDK::TScriptInterface<SDK::IBlendableInterface> blendable{};
		blendable.ObjectPointer = material;
		blendable.InterfacePointer = material;
		screen->AddOrUpdateBlendable(blendable, on ? 1.f : 0.f);
		return true;
	}

	static SDK::UMaterialInterface* material_named(const std::string& name)
	{
		std::lock_guard lock(g_mutex);
		auto it = std::ranges::find(g_filters, name, &filter_material::name);
		return it != g_filters.end() ? it->material.get() : nullptr;
	}

	static void switch_to(const std::string& name)
	{
		std::string previous;
		{
			std::lock_guard lock(g_mutex);
			previous = g_on;
			g_on.clear();
		}
		if (!previous.empty())
		{
			if (auto material = material_named(previous))
				apply(material, false);
		}
		if (name.empty() || name == previous)
			return;

		auto material = material_named(name);
		if (!material)
		{
			notification::warning("Visual Filter", "That material is no longer loaded; press Find Filters.");
			return;
		}

		if (apply(material, true))
		{
			std::lock_guard lock(g_mutex);
			g_on = name;
		}
	}

	std::vector<visual_filter::entry> visual_filter::list()
	{
		if (!g_asked.exchange(true))
			find();

		std::vector<entry> result;
		std::lock_guard lock(g_mutex);
		for (const auto& filter : g_filters)
			result.push_back({filter.name, filter.name == g_on});
		return result;
	}

	void visual_filter::find()
	{
		async_run([] {
			read_filters();
		});
	}

	void visual_filter::toggle(const std::string& name)
	{
		async_run([name] {
			switch_to(name);
		});
	}

	void visual_filter::turn_off()
	{
		async_run([] {
			switch_to({});
		});
	}
}
