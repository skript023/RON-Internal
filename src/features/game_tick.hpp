#pragma once

namespace big::game_tick
{
	// Called for every per-frame Blueprint event: ReceiveTick, BlueprintUpdateAnimation, or a widget's Tick, whichever
	// class overrides it. Resumes the tasks on the game thread, once per presented frame, or every 50 ms while nothing
	// is presented.
	void on_frame_event();
}
