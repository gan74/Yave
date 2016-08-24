/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/


#ifndef Y_UTILS_CHRONO_H
#define Y_UTILS_CHRONO_H

namespace y {

template<typename T>
struct Sec {
	T value;

	Sec(T v) : value(v) {
	}

	operator T() const {
		return value;
	}

	operator T&() {
		return value;
	}
};


template<typename T = double>
class Chrono {
	using Second = std::chrono::duration<T, std::ratio<1, 1>>;
public:
	Chrono() {
		start();
	}

	void start() {
		time = std::chrono::high_resolution_clock::now();
	}

	Sec<T> reset() {
		auto e = elapsed();
		start();
		return e;
	}

	Sec<T> elapsed() const {
		return std::chrono::duration_cast<Second>(std::chrono::high_resolution_clock::now() - time).count();
	}

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> time;
};


}

#endif //Y_CHRONO_H
