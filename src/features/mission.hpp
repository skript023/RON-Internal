#pragma once

namespace SDK
{
	class ACyberneticCharacter;
	class ADoor;
}

// Mission actions shared by the Mission tab, Auto Win, and the target inspector. Call them from the game thread;
// each returns how many things it changed.
namespace big::mission
{
	int arrest_all();
	int report_all();
	int collect_evidence();
	int surrender_suspects();
	int knockout_suspects();
	int unlock_doors();
	int open_doors();
	int disarm_traps();
	int remove_wedges();
	bool teleport_to_next_ai();

	void arrest(SDK::ACyberneticCharacter* character);
	void surrender(SDK::ACyberneticCharacter* character);
	void knockout(SDK::ACyberneticCharacter* character);
	void set_door_locked(SDK::ADoor* door, bool locked);
	void disarm_door_trap(SDK::ADoor* door);
}
