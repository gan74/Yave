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
#ifndef Y_CORE_ASSOCVECTOR_H
#define Y_CORE_ASSOCVECTOR_H

#include "Vector.h"

namespace y {
namespace core {

template<typename Key, typename Value, typename ResizePolicy = DefaultVectorResizePolicy>
class AssocVector : public Vector<std::pair<Key, Value>, ResizePolicy> {

	using Entry = std::pair<Key, Value>;
	using Base = Vector<Entry, ResizePolicy>;

	public:
		using Vector<Entry, ResizePolicy>::Vector;

		template<typename T>
		void insert(const Key& key, T&& value) {
			this->append(std::make_pair(key, std::forward<T>(value)));
		}

		Value& operator[](const Key& key) {
			for(Entry& e : *this) {
				if(e.first == key) {
					return e.second;
				}
			}
			insert(key, Value());
			return this->last().second;
		}
};

}
}

#endif // Y_CORE_ASSOCVECTOR_H
