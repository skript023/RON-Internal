#include "../view.hpp"
#include <astra/host/canvas.hpp>

namespace big
{
	void view::notifications()
	{
		canvas::draw_notifications();
	}
}
