#pragma once
#include "weak_object.hpp"
#include "SDK/ReadyOrNot_classes.hpp"

namespace big
{
	// The original stats of every weapon a feature changed, so turning it off puts each one back. A weapon the game
	// destroyed since, such as on a level change, is skipped instead of written to.
	template <typename Backup>
	class weapon_backup
	{
	public:
		using entry = std::pair<weak_object<SDK::ABaseMagazineWeapon>, Backup>;

		bool contains(SDK::ABaseMagazineWeapon* weapon) const
		{
			return std::ranges::any_of(m_entries, [weapon](const entry& e) {
				return e.first.address() == weapon && e.first.get() == weapon;
			});
		}

		void add(SDK::ABaseMagazineWeapon* weapon, Backup backup)
		{
			m_entries.emplace_back(weapon, std::move(backup));
		}

		auto begin()
		{
			return m_entries.begin();
		}

		auto end()
		{
			return m_entries.end();
		}

		void clear()
		{
			m_entries.clear();
		}

	private:
		std::vector<entry> m_entries;
	};

	template <typename Backup>
	class ammo_backup
	{
	public:
		static int loaded_type(SDK::ABaseMagazineWeapon* weapon)
		{
			return weapon->GetCurrentMagazine().AmmoType;
		}

		bool contains(SDK::ABaseMagazineWeapon* weapon, int type) const
		{
			return std::ranges::any_of(m_entries, [weapon, type](const entry& e) {
				return e.type == type && e.weapon.address() == weapon && e.weapon.get() == weapon;
			});
		}

		void add(SDK::ABaseMagazineWeapon* weapon, int type, Backup backup)
		{
			m_entries.push_back({weapon, type, std::move(backup)});
		}

		template <typename F>
		void restore_loaded(F&& restore)
		{
			for (auto& e : m_entries)
			{
				if (auto weapon = e.weapon.get(); weapon && loaded_type(weapon) == e.type)
					restore(weapon, e.backup);
			}
			m_entries.clear();
		}

	private:
		struct entry
		{
			weak_object<SDK::ABaseMagazineWeapon> weapon;
			int type;
			Backup backup;
		};

		std::vector<entry> m_entries;
	};
}
