/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
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
		using value_type = MapEntry<Key, Value>;
		using Vector<value_type, ResizePolicy>::Vector;

		using iterator = typename Vector<value_type, ResizePolicy>::iterator;
		using const_iterator = typename Vector<value_type, ResizePolicy>::const_iterator;

		template<typename K, typename T>
		void insert(K&& key, T&& value) {
			this->push_back(value_type(std::forward<K>(key), std::forward<T>(value)));
		}

		Value& operator[](const Key& key) {
			for(value_type& e : *this) {
				if(e == key) {
					return e.second;
				}
			}
			insert(key, Value());
			return this->last().second;
		}

		iterator find(const Key& key) {
			return std::find_if(this->begin(), this->end(), [&](const auto& k) { return k == key; });
		}

		const_iterator find(const Key& key) const {
			return std::find_if(this->begin(), this->end(), [&](const auto& k) { return k == key; });
		}

	private:
};


}
}

#endif // Y_CORE_ASSOCVECTOR_H
