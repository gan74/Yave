
#include <yave/image/ImageData.h>
#include <yave/window/Window.h>


#include <yave/script/LuaComponent.h>

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


double random() {
	return rand() / double(RAND_MAX);
}

void pi() {
	usize inside = 0;
	usize total = 0;
	for(usize i = 0; i != 1000000; ++i) {
		double x = random();
		double y = random();
		if(x * x + y * y < 1.0) {
			inside++;
		}
		total++;
	}
	std::cout << (inside / double(total)) * 100 << "%\n";
}


int main(int, char **) {
	auto state = yave::lua::create_state();

	Chrono lua;
	state.do_string(R"#(
		inside = 0
		total = 0
		for i = 1, 1000000 do
			x = math.random()
			y = math.random()
			if x * x + y * y < 1 then
				inside = inside + 1
			end
			total = total + 1
		end
		print((inside / total) * 100 .. '%')
	)#");
	std::cout << "lua: " << lua.elapsed().to_millis() << "ms" << std::endl;

	Chrono cpp;
	pi();
	std::cout << "c++: " << cpp.elapsed().to_millis() << "ms" << std::endl;

	return 0;


	/*LuaComponent cmp(state, R"#(
			return {total = 0, update =
				function (self, dt)
					self.total = self.total + dt
					print(('dt: %.4f total: %.2f'):format(dt, self.total))
					--[[ if self.total > 3 then
						self.total = fail + 1
					end ]]--
				end}
		 )#");

	core::Chrono time;
	while(true) {
		cmp.update(time.reset());
	}

	return 0;*/


	Window win(math::vec(1280, 768), "Yave");
	win.set_mouse_handler(new ArcballMouse());

	YaveApp app(DebugParams::debug());
	app.init(&win);

	win.show();
	while(win.update()) {
		auto mouse = reinterpret_cast<ArcballMouse *>(win.mouse_handler());
		app.update(mouse->angle * 0.01);
		app.draw();
	}

	return 0;
}
