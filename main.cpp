
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

auto proj(float fov, float ratio, float zNear, float zFar) {
	float f = std::cos(fov / 2.0) / std::sin(fov / 2.0);
	float z = zFar - zNear;
	return math::Matrix4<>(f / ratio, 0, 0, 0,
									   0, f, 0, 0,
									   0, 0, -(zFar + zNear) / z, -1,
									   0, 0, -(2 * zFar * zNear) / z, 0);
}


template<usize N, typename T>
std::ostream& operator<<(std::ostream& out, const math::Vec<N, T>& vec) {
	out << "(";
	if(N) {
		for(usize i = 0; i < N - 1; i++) {
			out << vec[i] << ", ";
		}
		out << vec[N - 1];
	}
	return out << ")";
}

template<usize N, usize M, typename T>
std::ostream& operator<<(std::ostream& out, const math::Matrix<N, M, T>& mat) {
	out << "[";
	if(N) {
		for(usize i = 0; i < N - 1; i++) {
			out << mat[i] << ", ";
		}
		out << mat[N - 1];
	}
	return out << "]";
}

auto computeViewMatrix(math::Vec3 forward, math::Vec3 up, math::Vec3 side, math::Vec3 p) {
		return math::Matrix4<>(-side, side.dot(p),
									up, -up.dot(p),
									-forward, forward.dot(p),
			0, 0, 0, 1);
}

int main(int, char **) {
	Window win(math::vec(1280, 768), "Yave");
	win.set_mouse_handler(new ArcballMouse());

	YaveApp app(DebugParams::debug());
	app.init(&win);

	win.show();
	/*Chrono ch;
	usize frames = 0;*/

	while(win.update()) {
		auto mouse = reinterpret_cast<ArcballMouse *>(win.mouse_handler());
		app.update(mouse->angle * 0.01);
		app.draw();

		/*auto us = round(ch.reset().to_micros());
		std::cout << "\r";
		std::cout << std::setw(25) << std::left << ("frame time = "_s + us + "us");
		std::cout << std::setw(25) << std::left << ("(avg = "_s + round(avg(us)) + "us) f = " + (++frames));*/

	}
	std::cout << std::endl;

	return 0;
}
