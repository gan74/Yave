#include <yave/LowLevelGraphics.h>
#include <yave/Window.h>
#include <y/math/Vec.h>
#include <iostream>

#include "yave/image/ImageData.h"

#include <fstream>

using namespace yave;
using namespace core;

class ArcballMouse : public MouseEventHandler {
	public:
		math::Vec2 angle;

		ArcballMouse() : _dragging(false) {
		}

		virtual void mouse_moved(const math::Vec2i &pos) override {
			if(_dragging) {
				angle += pos - _last;
				_last = pos;
			}
		}

		virtual void mouse_pressed(const math::Vec2i &pos, MouseButton) override {
			_dragging = true;
			_last = pos;
		}

		virtual void mouse_released(const math::Vec2i &, MouseButton) override {
			_dragging = false;
		}

	private:
		math::Vec2i _last;
		bool _dragging;
};

int main(int, char **) {
	Window win(math::vec(1024, 728), "Yave");
	win.set_mouse_handler(new ArcballMouse());

	LowLevelGraphics graphics(DebugParams::debug());
	graphics.init(&win);

	win.show();


	while(win.update()) {
		auto mouse = reinterpret_cast<ArcballMouse *>(win.get_mouse_handler());
		graphics.update(mouse->angle * 0.01);
		graphics.draw();

	}

	return 0;
}
