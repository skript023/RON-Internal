#pragma once
#include "weak_object.hpp"

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
}
