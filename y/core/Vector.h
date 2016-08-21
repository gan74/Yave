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
#ifndef Y_CORE_VECTOR
#define Y_CORE_VECTOR

#include <y/utils.h>
#include <cstring>

namespace y {
namespace core {

struct DefaultVectorResizePolicy {
	static constexpr usize threshold = 4096;
	static constexpr usize step = 2048;

	usize ideal_capacity(usize size) {
		if(!size) {
			return 0;
		}
		if(size < threshold) {
			return 1 << (log2ui(size) + 1);
		}
		usize steps = (size - threshold) / step;
		return threshold + (steps + 1) * step;
	}

	bool shrink(usize size, usize capacity) {
		return !size || (capacity - size) > 2 * step;
	}
};


template<typename Elem, typename ResizePolicy = DefaultVectorResizePolicy>
class Vector : DefaultVectorResizePolicy {

	using Data = typename std::remove_const<Elem>::type;

	public:
		using iterator = Elem *;
		using const_iterator = Elem const *;
		using Element = Elem;

		Vector() : data(nullptr), dataEnd(nullptr), allocEnd(nullptr) {
		}

		template<typename RP>
		Vector(const Vector<Elem, RP> &other) : Vector() {
			set_min_capacity(other.size());
			for(const Elem &e : other) {
				append(e);
			}
		}

		~Vector() {
			clear();
		}

		void append(const Element &elem) {
			if(dataEnd == allocEnd) {
				expend();
			}
			new(dataEnd++) Data(elem);
		}

		usize size() const {
			return dataEnd - data;
		}

		bool isEmpty() const {
			return data == dataEnd;
		}

		usize capacity() const {
			return allocEnd - data;
		}

		const_iterator begin() const {
			return data;
		}

		const_iterator end() const {
			return dataEnd;
		}

		const_iterator cbegin() const {
			return data;
		}

		const_iterator cend() const {
			return dataEnd;
		}

		iterator begin() {
			return data;
		}

		iterator end() {
			return dataEnd;
		}

		const Element &operator[](usize i) const {
			return data[i];
		}

		Element &operator[](usize i) {
			return data[i];
		}

		void set_capacity(usize cap) {
			unsafe_set_capacity(size(), cap);
		}

		void set_min_capacity(usize min_cap) {
			unsafe_set_capacity(size(), ideal_capacity(min_cap));
		}

		void clear() {
			unsafe_set_capacity(0, 0);
		}

	private:
		void copy_range(Data *dst, const Data *src, usize n) {
			if(std::is_pod<Data>::value) {
				memcpy(dst, src, sizeof(Data) * n);
			} else {
				for(; n; n--) {
					new(dst++) Data(*(src++));
				}
			}
		}

		void move_range(Data *dst, const Data *src, usize n) {
			if(std::is_pod<Data>::value) {
				memmove(dst, src, sizeof(Data) * n);
			} else {
				for(; n; n--) {
					new(dst++) Data(std::move(*(src++)));
				}
			}
		}

		void clear(Data *src, usize n) {
			if(!std::is_pod<Data>::value) {
				while(n) {
					(src + --n)->~Data();
				}
			}
		}

		void clear(Data *beg, Data *en) {
			if(!std::is_pod<Data>::value) {
				while(en != beg) {
					(--en)->~Data();
				}
			}
		}

		void expend() {
			unsafe_set_capacity(size(), ideal_capacity(size() + 1));
		}

		void shrink() {
			usize current = size();
			usize cap = capacity();
			if(current < cap && shrink(current, cap)) {
				unsafe_set_capacity(current, ideal_capacity(current));
			}
		}

		// uses dataEnd !!
		void unsafe_set_capacity(usize num_to_move, usize new_cap) {
			using byte = u8;

			num_to_move = num_to_move < new_cap ? num_to_move : new_cap;

			Data *new_data = new_cap ? reinterpret_cast<Data *>(new byte[new_cap * sizeof(Data)]) : nullptr;

			move_range(new_data, data, num_to_move);
			clear(data, dataEnd);

			delete[] reinterpret_cast<byte *>(data);
			data = new_data;
			dataEnd = data + num_to_move;
			allocEnd = data + new_cap;
		}

		Data *data;
		Data *dataEnd;
		Data *allocEnd;


};

}
}



#endif
