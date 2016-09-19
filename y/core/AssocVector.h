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
template<typename Key, typename Value>
struct MapEntry : public std::pair<Key, Value> {
	using std::pair<Key, Value>::pair;

	constexpr bool operator==(const Key& k) const {
		return this->first == k;
	}

	constexpr bool operator==(const MapEntry& e) const {
		return this->first == e;
	}
};


template<typename Key, typename Value, typename ResizePolicy = DefaultVectorResizePolicy>
class AssocVector : public Vector<MapEntry<Key, Value>, ResizePolicy> {

	public:
		using Entry = MapEntry<Key, Value>;
		using Vector<Entry, ResizePolicy>::Vector;

		template<typename T>
		void insert(const Key& key, T&& value) {
			this->append(std::make_pair(key, std::forward<T>(value)));
		}

		Value& operator[](const Key& key) {
			for(Entry& e : *this) {
				if(e == key) {
					return e.second;
				}
			}
			insert(key, Value());
			return this->last().second;
		}

	private:
};


}
}

#endif // Y_CORE_ASSOCVECTOR_H
