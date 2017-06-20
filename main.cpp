
#include <yave/image/ImageData.h>
#include <yave/window/Window.h>


#include <y/math/Vec.h>

#include <iostream>
#include <iomanip>

#include "YaveApp.h"

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
			_last = pos;
			_dragging = true;
		}

		virtual void mouse_released(const math::Vec2i &, MouseButton) override {
			_dragging = false;
		}

	private:
		math::Vec2i _last;
		bool _dragging;
};

int main(int, char **) {
	log_msg("starting...");
	Window win({1280, 768}, "Yave");
	win.set_mouse_handler(new ArcballMouse());

	YaveApp app(DebugParams::debug());
	app.set_swapchain(&win);

	win.show();
	while(win.update()) {
		auto mouse = reinterpret_cast<ArcballMouse *>(win.mouse_handler());
		app.update(mouse->angle * 0.01);
		app.draw();
	}

	return 0;
}
