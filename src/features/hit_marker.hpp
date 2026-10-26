#pragma once
#include "SDK/ReadyOrNot_classes.hpp"

// A mark on the crosshair for each hit of yours, from the damage the game reports.
namespace big::hit_marker
{
	// Game thread: a hit of yours on someone, for the given damage.
	void add(SDK::AReadyOrNotCharacter* victim, float damage);
	// Render thread: the mark and the damage of recent hits.
	void draw();
}
