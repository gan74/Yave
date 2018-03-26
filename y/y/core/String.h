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
#ifndef Y_CORE_STRING_H
#define Y_CORE_STRING_H

#include <y/utils.h>
#include <sstream>

namespace y {
namespace core {

// see: https://www.youtube.com/watch?v=kPR8h4-qZdk
class String {

	struct LongLenType
	{
		usize _len : 8 * sizeof(usize) - 1;
		usize _is_long : 1;

		LongLenType(usize l = 0) : _len(l), _is_long(1) {
		}

		operator usize() const {
			return _len;
		}
	};

	struct ShortLenType
	{
		u8 _len : 7;
		u8 _is_long : 1;

		// Y_TODO(SSO implementation squeeze an extra byte at the cost of 0 initilisation. Bench needed)
		ShortLenType(usize l = 0) : _len(u8(max_short_size - l)), _is_long(0) {
		}

		operator usize() const {
			return max_short_size - _len;
		}
	};

	struct LongData
	{
		Owner<char*> data;
		usize capacity;
		LongLenType length;

		LongData();
		LongData(const LongData& _l);
		LongData(LongData&& _l);
		LongData(const char* str, usize cap, usize len);
		LongData(const char* str, usize len);

		LongData& operator=(const LongData &) = delete;
	};

	struct ShortData
	{
		char _data[sizeof(LongData) - 1];
		ShortLenType _length;

		ShortData();
		ShortData(const ShortData& _s);
		ShortData(const char* str, usize len);

		ShortData& operator=(const ShortData &) = delete;

	};

	static_assert(sizeof(ShortData) == sizeof(LongData), "String::LongData should be the same length as String::ShortData");

	public:
		static constexpr usize max_short_size = sizeof(ShortData::_data);

		using value_type = char;
		using size_type = usize;

		using iterator = char*;
		using const_iterator = const char*;

		String();
		String(const String& str);
		String(String&& str);
		String(const std::string& str);
		String(std::string_view str);

		String(const char* str);
		String(const char* str, usize len);
		String(const char* beg, const char* end);


		template<typename It>
		String(It beg_it, It end_it) : String(nullptr, std::distance(beg_it, end_it)) {
			std::copy(beg_it, end_it, begin());
		}

		~String();

		// String take ownership
		static String from_owned(Owner<char*> owned);

		void set_min_capacity(usize cap);

		usize size() const;
		usize capacity() const;
		bool is_empty() const;
		bool is_long() const;

		void clear();

		char* data();
		const char* data() const;

		iterator find(const char* str);
		const_iterator find(const char* str) const;

		String sub_str(usize beg) const;
		String sub_str(usize beg, usize len) const;

		operator const char*() const;
		operator char*();

		// to prevent Strings converting to bool via operator char*
		explicit operator bool() = delete;

		void swap(String& str);


		std::string_view view() const;
		operator std::string_view() const;


		String& operator+=(const String& str);
		String& operator+=(const char* str);
		String& operator+=(const std::string& str);
		String& operator+=(std::string_view str);

		char& operator[](usize i);
		char operator[](usize i) const;

		bool operator==(const char* str) const;
		bool operator!=(const char* str) const;

		bool operator==(const String& str) const;
		bool operator!=(const String& str) const;
		bool operator<(const String& str) const;


		template<typename T>
		String& operator=(T&& t) {
			return operator=(String(std::forward<T>(t)));
		}

		String& operator=(const String& str);
		String& operator=(String&& str);

		iterator begin() {
			return data();
		}

		iterator end() {
			return data() + size();
		}

		const_iterator begin() const {
			return data();
		}

		const_iterator end() const {
			return data() + size();
		}

		const_iterator cbegin() const {
			return data();
		}

		const_iterator cend() const {
			return data() + size();
		}

	private:
		union
		{
			LongData _l;
			ShortData _s;
		};

		const String* const_this() {
			return this;
		}

		String& append(const char* other_data, usize other_size);

		static char* alloc_long(usize capacity);
		static usize compute_capacity(usize len);
		static void free_long(LongData& d);

};










template<typename T>
String str(T&& t) {
	if constexpr (std::is_convertible_v<T, String>) {
		return String(std::forward<T>(t));
	} else {
		std::ostringstream oss;
		oss << std::forward<T>(t);
		return oss.str();
	}
}


namespace detail {
template<typename T>
using has_append_t = decltype(std::declval<String&>().operator+=(std::declval<const T&>()));
}

template<typename T>
core::String& operator+(core::String l, T&& r) {
	if constexpr(is_detected_v<detail::has_append_t, T>) {
		return l += std::forward<T>(r);
	} else {
		return l += str(std::forward<T>(r));
	}
}


} // core


inline core::String operator "" _s(const char* c_str, usize size) {
	return core::String(c_str, size);
}




// because we do need string in here
namespace detail {
core::String demangle_type_name(const char* name);
}

template<typename T>
auto type_name() {
	return detail::demangle_type_name(typeid(T).name()) + (std::is_reference_v<T> ? "&" : "");
}

template<typename T>
auto type_name(T&& t) {
	return detail::demangle_type_name(typeid(std::forward<T>(t)).name());
}

}

namespace std {
template<>
struct hash<y::core::String> : private std::hash<std::string_view> {
	auto operator()(const y::core::String& str) const {
		return std::hash<std::string_view>::operator()(str);
	}
};
}

#endif // Y_CORE_STRING_H
