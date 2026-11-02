#pragma once

// A crosshair on what your gun points at, wherever the camera sits, such as over your shoulder in Third Person.
namespace big::crosshair
{
	// Render thread: the crosshair, while it is on.
	void draw();
	// Render thread: where the crosshair is drawn, when it is on and away from the middle of the screen.
	bool position(float& x, float& y);
}
