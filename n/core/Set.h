/*******************************
Copyright (C) 2013-2014 grégoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied wtreeanty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/

#ifndef N_CORE_SET_H
#define N_CORE_SET_H

#include "RBTree.h"

namespace n {
namespace core {

template<typename T>
using Set = RBTree<T, std::less<T>, std::equal_to<T>>;

} //core
} //n

#endif // N_CORE_SET_H
