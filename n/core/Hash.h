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

#ifndef N_CORE_HASH
#define N_CORE_HASH

#include "Array.h"
#include "Pair.h"
#include <n/utils.h>

namespace n {
namespace core {

template<typename T, typename U>
class Hash
{

	public:
		typedef Pair<T, U> element;

		Hash() : tSize(0) {
			allocTable(93);
		}

		void insert(const T &t, const U &u) {
			return insert(element(t, u));
		}

		void insert(const element &e) {
			auto h = hash(e._1) % values.size();
			values[h].append(e);
			tSize++;
		}

		U &operator[](const T &t) {
			auto h = hash(t) % values.size();
			Array<element> &arr = values[h];
			typename Array<element>::iterator it = arr.findOne([&](const element &e) { return e._1 == t; });
			if(it == arr.end()) {
				arr.append(element(t, U()));
				it = arr.end();
				it--;
				tSize++;
			}
			return it->_2;
		}

		const U &get(const T &t, const U &def) const {
			auto h = hash(t) % values.size();
			const Array<element> &arr = values[h];
			typename Array<element>::const_iterator it = arr.findOne([&](const element &e) { return e._1 == t; });
			return it == arr.end() ? def : it->_2;
		}

		const U &get(const T &t) const {
			return get(t, U());
		}

		uint size() const {
			return tSize;
		}

	private:
		void rehash(uint si) {
			Array<Pair<T, U>> all(values);
			allocTable(si);
			for(const element &p : all) {
				auto h = hash(p._1) % values.size();
				values[h].append(p);
			}
		}

		void allocTable(uint si) {
			values.makeEmpty();
			values.setCapacity(si);
			for(uint i = 0; i != si; i++) {
				values.append(Array<element>());
			}
		}

		uint tSize;
		Array<Array<element>> values;
};

}
}

#endif // N_CORE_HASH

