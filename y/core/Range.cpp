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

#include "Range.h"
#include "Vector.h"
#include <y/test/test.h>

namespace y {
namespace core {

static_assert(std::is_same<typename decltype(range(vector(1, 2, 3)))::Element, int>::value, "Range::Element deduction error");


}
}
