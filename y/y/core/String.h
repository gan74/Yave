/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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

#include <string>
#include <string_view>

namespace y {
namespace core {

// see: https://www.youtube.com/watch?v=kPR8h4-qZdk
class String {

    struct LongLenType {
        usize _len : 8 * sizeof(usize) - 1;
        usize _is_long : 1;

        LongLenType(usize l = 0) : _len(l), _is_long(1) {
        }

        operator usize() const {
            return _len;
        }

        static constexpr usize max_length() {
            return (1_uu << (8 * sizeof(usize) - 1)) - 2;
        }
    };

    struct ShortLenType {
        u8 _len : 7;
        u8 _is_long : 1;

        // Y_TODO(SSO implementation squeeze an extra byte at the cost of 0 initialisation. Bench needed)
        ShortLenType(usize l = 0) : _len(u8(max_short_size - l)), _is_long(0) {
        }

        operator usize() const {
            return max_short_size - _len;
        }
    };

    struct LongData {
        Owner<char*> data;
        usize capacity;
        LongLenType length;

        inline LongData();
        inline LongData(LongData&& other);
        inline LongData(const char* str, usize cap, usize len);
        inline LongData(const char* str, usize len);

        ~LongData() = default;

        inline LongData& operator=(LongData&& other);
        LongData& operator=(const LongData&) = delete;

        inline void swap(LongData& other);
    };

    struct ShortData {
        char data[sizeof(LongData) - 1];
        ShortLenType length;

        inline ShortData();
        ShortData(const ShortData&) = default;

        inline ShortData(const char* str, usize len);

        const ShortData& operator=(const ShortData &) = delete;
        ShortData& operator=(ShortData&& other) = default;

    };

    static_assert(sizeof(ShortData) == sizeof(LongData), "String::LongData should be the same length as String::ShortData");

    public:
        static constexpr usize max_short_size = sizeof(ShortData::data);

        using value_type = char;
        using size_type = usize;

        using iterator = char*;
        using const_iterator = const char*;

        inline String();
        inline String(const String& str);
        inline String(String&& str);
        inline String(const std::string& str);
        inline String(std::string_view str);

        inline String(const char* str);
        inline String(const char* str, usize len);
        inline String(const char* beg, const char* end);


        template<typename It>
        String(It beg_it, It end_it) : String(nullptr, std::distance(beg_it, end_it)) {
            std::copy(beg_it, end_it, begin());
        }


        String(nullptr_t) = delete;


        inline ~String();


        inline void set_min_capacity(usize cap);

        inline usize size() const;
        inline usize capacity() const;
        inline bool is_empty() const;
        inline bool is_long() const;

        inline void clear();
        inline void make_empty();
        inline void shrink(usize new_size);
        inline void grow(usize new_size, char c);
        inline void resize(usize new_size, char c = ' ');

        static inline String replaced(std::string_view str, std::string_view from, std::string_view to);
        inline String replaced(std::string_view from, std::string_view to) const;

        inline char* data();
        inline const char* data() const;

        inline iterator find(std::string_view str);
        inline const_iterator find(std::string_view str) const;

        inline std::string_view sub_str(usize beg) const;
        inline std::string_view sub_str(usize beg, usize len) const;

        inline bool starts_with(std::string_view str) const;
        inline bool ends_with(std::string_view str) const;

        inline explicit operator const char*() const;
        inline explicit operator char*();

        // to prevent Strings converting to bool via operator char*
        explicit operator bool() = delete;

        inline void swap(String& str);


        inline std::string_view view() const;
        inline operator std::string_view() const;

        inline void push_back(char c);

        inline String& operator+=(const String& str);
        inline String& operator+=(const char* str);
        inline String& operator+=(const std::string& str);
        inline String& operator+=(std::string_view str);
        // char deliberately excluded (causes problem when cat-ing numbers);

        inline char& operator[](usize i);
        inline char operator[](usize i) const;

        inline bool operator==(const char* str) const;
        inline bool operator!=(const char* str) const;

        inline bool operator==(const String& str) const;
        inline bool operator!=(const String& str) const;
        inline bool operator<(const String& str) const;

        inline bool operator==(std::string_view str) const;
        inline bool operator!=(std::string_view str) const;
        inline bool operator<(std::string_view str) const;

        template<typename T>
        String operator+(T&& t) const requires requires(String& s) { s += y_fwd(t); } {
            String s(*this);
            s += y_fwd(t);
            return s;
        }

        template<typename T>
        String& operator=(T&& t) {
            return operator=(String(y_fwd(t)));
        }

        inline String& operator=(const String& str);
        inline String& operator=(String&& str);

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

        static constexpr usize max_size() {
            return LongLenType::max_length();
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

        inline String& append(const char* other_data, usize other_size);

        static inline char* alloc_long(usize capacity);
        static inline usize compute_capacity(usize len);
        static inline void free_long(LongData& d);
        static inline void free_short(ShortData& d);
        inline void set(const char* str, usize len);

        inline void free_data();

};


inline std::string_view trim_left(std::string_view str);
inline std::string_view trim_right(std::string_view str);
inline std::string_view trim(std::string_view str);


} // core

inline core::String operator+(std::string_view l, const core::String& r);


template<usize N>
struct FixedString {
        char chars[N];

        constexpr FixedString(const char (&str)[N]) {
            for (usize i = 0; i != N; ++i) {
                chars[i] = str[i];
            }
        }

        constexpr usize size() const {
            return N - 1;
        }

        constexpr const char* data() const {
            return chars;
        }

        constexpr operator std::string_view() const {
            return {chars, N - 1};
        }
};

template<usize N>
FixedString(const char (&)[N]) -> FixedString<N>;

}


template<>
struct std::hash<y::core::String> : private std::hash<std::string_view> {
    auto operator()(const y::core::String& str) const {
        return std::hash<std::string_view>::operator()(str);
    }
};


#include "String.inl"

#endif // Y_CORE_STRING_H

