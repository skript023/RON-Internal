#pragma once
#include <string>
#include <vector>

namespace big::visual_filter
{
	struct entry
	{
		std::string name;
		bool on = false;
	};

	// The post-process materials the game has loaded, as last read. Called from the render thread; the first call reads
	// them on the game thread.
	std::vector<entry> list();

	// Reads them again, since the game loads more of them as it goes.
	void find();

	// Called from the render thread; the filter changes on the game thread. Only one is on at a time.
	void toggle(const std::string& name);
	void turn_off();
}
