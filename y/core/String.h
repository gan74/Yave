/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
#include "Vector.h"
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

		/*usize operator=(usize l) {
			return _len = l;
		}*/

		operator usize() const {
			return _len;
		}
	};

	struct ShortLenType
	{
		u8 _len : 7;
		u8 _is_long : 1;

		Y_TODO(SSO implementation squeeze an extra byte at the cost of 0 initilisation. Bench needed)
		ShortLenType(usize l = 0) : _len(u8(MaxShortSize - l)), _is_long(0) {
		}

		/*usize operator=(usize l) {
			_len = u8(MaxShortSize - l);
			return l;
		}*/

		operator usize() const {
			return MaxShortSize - _len;
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
		static constexpr usize MaxShortSize = sizeof(ShortData::_data);

		using iterator = char*;
		using const_iterator = const char*;

		String();
		String(const String& str);
		String(String&& str);

		String(const char* str); // NOT explicit
		String(const char* str, usize len);
		String(const char* beg, const char* end);

		template<typename I, typename = typename std::enable_if<std::is_same<typename Range<I>::Element, char>::value>::type>
		String(const Range<I>& rng);

		~String();

		// the pointer is not owned anymore, String take ownership;
		static String from_owned(Owner<char*> owned);

		template<typename T>
		static String from(T&& t);

		//auto parse() const;

		usize size() const;
		usize capacity() const;
		bool is_empty() const;
		bool is_long() const;

		void clear();

		char* data();
		const char* data() const;

		iterator find(const String& str);
		const_iterator find(const String& str) const;

		String sub_str(usize beg) const;
		String sub_str(usize beg, usize len) const;


		operator const char*() const;
		operator char*();

		// to prevent Strings converting to bool via operator char*
		explicit operator bool() = delete;

		void swap(String& str);

		String& operator=(const String& str);
		String& operator=(String&& str);

		String& operator+=(const String& str);

		char& operator[](usize i);
		char operator[](usize i) const;

		bool operator==(const String& str) const;
		bool operator!=(const String& str) const;
		bool operator<(const String& str) const;


		//Vector<u32> to_unicode() const;



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

		static char* alloc_long(usize capacity);
		static usize compute_capacity(usize len);
		static void free_long(LongData& d);

};



namespace detail {
template<typename T>
inline String str(const T& t, std::false_type) {
	return String::from(t);
}

template<typename T>
inline String str(const T& t, std::true_type) {
	return String(t);
}
}

template<typename... Args>
inline String str(Args... args) {
	return String(args...);
}

template<typename T>
inline String str(const T& t) {
	return detail::str(t, std::is_convertible<T, String>());
}

inline String str_from_owned(Owner<char*> owned) {
	return String::from_owned(owned);
}

template<typename T>
inline String str_from(const T& t) {
	return String::from(t);
}




template<typename T>
String String::from(T&& t) {
	std::ostringstream oss;
	oss << std::forward<T>(t);
	return oss.str().c_str();
}

template<typename I, typename = typename std::enable_if<std::is_same<typename Range<I>::Element, char>::value>::type>
String::String(const Range<I>& rng) : String(0, rng.size()) {
	char* it = begin();
	for(const auto& e : rng) {
		*(it++) = e;
	}
	*it = 0;
}

/*namespace detail {
class StringParser : NonCopyable {
	friend class core::String;
	StringParser(const String& str) : _str(str) {
	}

	const String& _str;

	public:
		template<typename T>
		operator T() const && {
			T t;
			std::stringstream buff;
			buff << _str;
			buff >> t;
			return t;
		}
};
}

inline auto String::parse() const {
	return detail::StringParser(*this);
}*/



template<typename T>
auto operator+(core::String l, const T& r) {
	return l += str(r);
}

template<typename T>
auto operator<<(core::String& l, const T& r) {
	l += str(r);
	return l;
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
	Y_TODO(disable this if no-rtti)
	return detail::demangle_type_name(typeid(T).name()) + (std::is_reference<T>::value ? "&" : "");
}

}


namespace std {

template<>
struct hash<y::core::String> {
	typedef y::core::String argument_type;
	typedef std::size_t result_type;
	result_type operator()(const argument_type& str) const {
		return y::hash(str);
	}
};

}

#endif // Y_CORE_STRING_H
