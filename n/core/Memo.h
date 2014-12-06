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

#ifndef N_CORE_MEMO_H
#define N_CORE_MEMO_H

#include <map>
#include <n/types.h>
#include <n/utils.h>

namespace n {
namespace core {

template<typename T>
class MemFunc : public NonCopyable
{
	public:
		MemFunc(const T &&w) : t(w) {
		}

		template<typename... Args>
		typename std::result_of<T(Args...)>::type operator()(Args... args) {
			static std::map<std::tuple<Args...>, typename std::result_of<T(Args...)>::type> map;
			auto it = map.find(std::make_tuple(args...));
			if(it == map.end()) {
				return map[std::make_tuple(args...)] = t(args...);
			}
			return it->second;
		}

	private:
		T t;
};

template<typename T>
MemFunc<typename TypeInfo<T>::nonRef> Memo(T&& t) {
	return MemFunc<typename TypeInfo<T>::nonRef>(std::forward<T>(t));
}

} //core
} //n


#endif // N_CORE_MEMO_H
