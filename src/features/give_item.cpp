#include "menu/view.hpp"
#include "commands/bool_command.hpp"
#include "renderer.hpp"
#include "task_mgr.hpp"
#include "utility/ron.hpp"
#include "SDK/ReadyOrNot_classes.hpp"
#include "SDK/Engine_parameters.hpp"

#include <astra/components.hpp>
#include <astra/host/canvas.hpp>

namespace big
{
	static bool_command _give_item_window("give_item_window", "Give Item", "A panel of the loadout's weapons and gear; Give puts one in your inventory (host only)", false);

	// Icons are drawn square with the game's wide pictures centred in them, so the list shows them unstretched.
	constexpr int icon_size = 128;
	using icon_pixels = std::shared_ptr<const std::vector<std::uint8_t>>;

	struct item_entry
	{
		std::string name;
		const char* kind;
		weak_object<SDK::UClass> type;
		icon_pixels icon; // RGBA, empty until it is drawn
	};

	// Read on the game thread from the loadout lists the player picks from; the panel draws from a copy.
	static std::mutex g_mutex;
	static std::vector<item_entry> g_items;
	static std::vector<const char*> g_kinds; // "All", then each kind in the order the loadout lists show them
	static std::atomic<bool> g_loading = false;

	static const char* kind_name(SDK::EItemClass kind)
	{
		using enum SDK::EItemClass;
		switch (kind)
		{
		case IC_AssaultRifle: return "Assault Rifle";
		case IC_SMG: return "SMG";
		case IC_LMG: return "LMG";
		case IC_Pistol: return "Pistol";
		case IC_Sniper: return "Sniper";
		case IC_Melee: return "Melee";
		case IC_LessLethal: return "Less Lethal";
		case IC_Shotgun: return "Shotgun";
		case IC_Launcher: return "Launcher";
		case IC_Grenade: return "Grenade";
		case IC_Shield: return "Shield";
		case IC_TacticalDevice: return "Tactical Device";
		case IC_LongTactical: return "Long Tactical";
		default: return "Other";
		}
	}

	// The engine allocates the samples, so one array is handed back each time and refilled in place, rather than a new
	// one left behind per read.
	static SDK::TArray<SDK::FColor> g_samples;

	static bool read_render_target(SDK::UWorld* world, SDK::UTextureRenderTarget2D* target)
	{
		static auto function = SDK::UKismetRenderingLibrary::StaticClass()->GetFunction("KismetRenderingLibrary", "ReadRenderTarget");
		SDK::Params::KismetRenderingLibrary_ReadRenderTarget parms{};
		parms.WorldContextObject = world;
		parms.TextureRenderTarget = target;
		parms.OutSamples = g_samples;
		parms.bNormalize = true;

		const auto flags = function->FunctionFlags;
		function->FunctionFlags |= 0x400;
		SDK::UKismetRenderingLibrary::GetDefaultObj()->ProcessEvent(function, &parms);
		function->FunctionFlags = flags;

		g_samples = parms.OutSamples;
		return parms.ReturnValue;
	}

	// The item's loadout picture, drawn onto the render target and read back as RGBA.
	static icon_pixels draw_icon(SDK::UWorld* world, SDK::UTextureRenderTarget2D* target, SDK::ABaseItem* item)
	{
		auto asset = SDK::UKismetSystemLibrary::LoadAsset_Blocking(reinterpret_cast<const SDK::TSoftObjectPtr<SDK::UObject>&>(item->ItemIcon));
		if (!asset || !asset->IsA(SDK::UTexture2D::StaticClass()))
			return nullptr;

		const auto texture = static_cast<SDK::UTexture2D*>(asset);
		const double width = texture->Blueprint_GetSizeX();
		const double height = texture->Blueprint_GetSizeY();
		if (width <= 0 || height <= 0)
			return nullptr;

		const double scale = icon_size / (std::max)(width, height);
		const SDK::FVector2D size{width * scale, height * scale};
		SDK::UKismetRenderingLibrary::ClearRenderTarget2D(world, target, {0.f, 0.f, 0.f, 0.f});
		SDK::UCanvas* canvas = nullptr;
		SDK::FVector2D canvas_size{};
		SDK::FDrawToRenderTargetContext context{};
		SDK::UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(world, target, &canvas, &canvas_size, &context);
		if (canvas)
			canvas->K2_DrawTexture(texture, {(icon_size - size.X) / 2, (icon_size - size.Y) / 2}, size, {0, 0}, {1, 1}, {1.f, 1.f, 1.f, 1.f},
			    SDK::EBlendMode::BLEND_Opaque, 0.f, {0.5, 0.5});
		SDK::UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(world, context);

		if (!read_render_target(world, target) || g_samples.Num() != icon_size * icon_size)
			return nullptr;

		auto pixels = std::make_shared<std::vector<std::uint8_t>>(icon_size * icon_size * 4);
		for (int i = 0; i < g_samples.Num(); i++)
		{
			const auto& color = g_samples[i];
			auto pixel = pixels->data() + i * 4;
			pixel[0] = color.R;
			pixel[1] = color.G;
			pixel[2] = color.B;
			pixel[3] = color.A;
		}
		return pixels;
	}

	static task<void> load_items()
	{
		auto world = SDK::UWorld::GetWorld();
		auto manager = world ? SDK::ULoadoutManager::Get(world) : nullptr;
		std::vector<item_entry> items;
		std::vector<weak_object<SDK::ABaseItem>> defaults;
		std::vector<const char*> kinds{"All"};
		if (manager)
		{
			for (auto list : {&manager->PrimaryWeapons, &manager->SecondaryWeapons, &manager->LongTacticalItems, &manager->TacticalItems})
			{
				for (auto item : *list)
				{
					if (!item)
						continue;
					const auto kind = kind_name(item->ItemClass);
					if (std::find(kinds.begin(), kinds.end(), kind) == kinds.end())
						kinds.push_back(kind);
					items.push_back({ron::text(item->ItemName), kind, item->Class, nullptr});
					defaults.emplace_back(item);
				}
			}
		}

		{
			std::lock_guard lock(g_mutex);
			g_items = items;
			g_kinds = std::move(kinds);
		}

		// One icon a frame, so drawing them does not stall the game.
		weak_object<SDK::UTextureRenderTarget2D> target;
		for (std::size_t i = 0; i < defaults.size(); i++)
		{
			co_await this_task::wait();
			world = SDK::UWorld::GetWorld();
			auto item = defaults[i].get();
			if (!world || !item)
				continue;

			auto canvas = target.get();
			if (!canvas)
			{
				canvas = SDK::UKismetRenderingLibrary::CreateRenderTarget2D(world, icon_size, icon_size, SDK::ETextureRenderTargetFormat::RTF_RGBA8_SRGB,
				    {0.f, 0.f, 0.f, 0.f}, false, false);
				target = canvas;
			}
			auto icon = canvas ? draw_icon(world, canvas, item) : nullptr;

			// The list may have been read again meanwhile, so the icon goes only to the same item.
			std::lock_guard lock(g_mutex);
			if (i < g_items.size() && g_items[i].type == items[i].type)
				g_items[i].icon = std::move(icon);
		}
		if (auto canvas = target.get())
			SDK::UKismetRenderingLibrary::ReleaseRenderTarget2D(canvas);
		g_loading = false;
	}

	static void give(const item_entry& entry)
	{
		if (!ron::require_host(_give_item_window.get_label()))
			return;

		auto c = unreal_engine::get_character();
		if (!c)
		{
			notification::warning(_give_item_window.get_label(), "Be in a mission first.");
			return;
		}

		auto type = entry.type.get();
		if (!type)
		{
			// The game let the item go since the list was read, so the panel reads the lists again.
			std::lock_guard lock(g_mutex);
			g_items.clear();
			notification::warning(_give_item_window.get_label(), std::format("The {} is no longer loaded; the list was read again.", entry.name));
			return;
		}

		if (ron::give_item(c, type))
			notification::success(_give_item_window.get_label(), std::format("{} is in your inventory.", entry.name));
		else
			notification::warning(_give_item_window.get_label(), std::format("Your inventory did not take the {}.", entry.name));
	}

	void view::give_item_panel()
	{
		if (!_give_item_window.get_state())
			return;

		std::vector<item_entry> items;
		std::vector<const char*> kinds;
		{
			std::lock_guard lock(g_mutex);
			items = g_items;
			kinds = g_kinds;
		}
		// The loadout manager may not exist yet, such as on the main menu, so an empty list is asked for again.
		if (items.empty() && !g_loading.exchange(true))
			async_run(load_items);

		// Each icon is uploaded once on the render thread, which owns the device; holding its pixels keeps the key unique.
		static std::map<icon_pixels, ImTextureID> textures;
		auto texture_of = [](const icon_pixels& icon) -> ImTextureID {
			if (!icon)
				return {};
			auto [it, added] = textures.try_emplace(icon, ImTextureID{});
			if (added)
				it->second = g_renderer->upload_rgba(icon->data(), icon_size, icon_size);
			return it->second;
		};

		bool open = true;
		if (canvas::begin_panel("Give Item", &open, {.size = {480.f, 620.f}}))
		{
			static int kind = 0;
			if (kind >= static_cast<int>(kinds.size()))
				kind = 0;
			if (!kinds.empty())
				astra::choice("##give_item_kind", &kind, kinds);
			static char search[64]{};
			astra::search_field("give_item_search", search, sizeof(search), "Search item name");

			std::vector<const item_entry*> found;
			for (const auto& entry : items)
			{
				if ((kind == 0 || entry.kind == kinds[kind]) && astra::matches_search(entry.name, search))
					found.push_back(&entry);
			}

			constexpr float preview = 96.f;
			static const void* selected = nullptr;
			const int clicked = astra::match_list("give_items", static_cast<int>(found.size()), [&](int i) {
				return astra::list_item{found[i]->name, found[i]->type.address() == selected, texture_of(found[i]->icon), found[i]->kind};
			}, {.height = -(preview + ImGui::GetStyle().ItemSpacing.y), .icon_size = 40.f, .count_label = "Items",
			    .empty_text = items.empty() ? "Loading the loadout lists..." : "No matches."});
			if (clicked >= 0)
				selected = found[clicked]->type.address();

			const auto pick = std::find_if(items.begin(), items.end(), [](const item_entry& entry) { return entry.type.address() == selected; });
			if (pick != items.end())
			{
				if (const auto texture = texture_of(pick->icon))
					ImGui::Image(texture, {preview, preview});
				else
					ImGui::Dummy({preview, preview});
				ImGui::SameLine();
				ImGui::BeginGroup();
				astra::text_accent("%s", pick->name.c_str());
				ImGui::TextUnformatted(pick->kind);
				if (astra::button("Give"))
					async_run([entry = *pick] { give(entry); });
				ImGui::EndGroup();
			}
			else
				ImGui::TextDisabled("Pick an item to see it here and give it.");
		}
		canvas::end_panel();

		// The close button switches the feature off, the same as the menu row.
		if (!open)
			_give_item_window.set_state(false);
	}
}
