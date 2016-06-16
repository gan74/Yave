/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

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

#ifndef N_MATH_VOLUME_H
#define N_MATH_VOLUME_H

#include "Vec.h"

namespace n {
namespace math {

template<typename T = float>
class Volume
{
	public:
		virtual bool isInside(const Vec<3, T> &v) const {
			return isInside(v, 0);
		}

		virtual bool isInside(const Vec<3, T> &, T) const = 0;
};

}
}

#endif // N_MATH_VOLUME_H

