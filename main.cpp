
#include <yave/image/ImageData.h>
#include <yave/Window.h>
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

template<typename T>
T avg(T t) {
	static u64 count = 0;
	static T total = T(0);
	count++;
	usize start = 512;
	if(count < start) {
		return total;
	}
	total += t;
	return total / (count - start);
}

int main(int, char **) {
	Window win(math::vec(1024, 728), "Yave");
	win.set_mouse_handler(new ArcballMouse());

	YaveApp app(DebugParams::debug());
	app.init(&win);

	win.show();

	Chrono ch;
	while(win.update()) {
		auto mouse = reinterpret_cast<ArcballMouse *>(win.get_mouse_handler());
		app.update(mouse->angle * 0.01);
		app.draw();

		auto us = round(ch.reset().to_micros());
		std::cout << "\r";
		std::cout << std::setw(25) << std::left << ("frame time = "_s + us + "us");
		std::cout << std::setw(25) << std::left << ("(avg = "_s + round(avg(us)) + "us)");

	}
	std::cout << std::endl;

	return 0;
}
