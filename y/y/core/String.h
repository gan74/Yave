/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
#include <y/utils/detect.h>

#include <string>
#include <string_view>

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

        static constexpr usize max_length() {
            return (1_uu << (8 * sizeof(usize) - 1)) - 2;
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
        LongData(LongData&& other);
        LongData(const char* str, usize cap, usize len);
        LongData(const char* str, usize len);

        ~LongData() = default;

        LongData& operator=(LongData&& other);
        LongData& operator=(const LongData&) = delete;

        void swap(LongData& other);
    };

    struct ShortData
    {
        char data[sizeof(LongData) - 1];
        ShortLenType length;

        ShortData();
        ShortData(const ShortData&) = default;

        ShortData(const char* str, usize len);

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


        String(nullptr_t) = delete;


        ~String();


        void set_min_capacity(usize cap);

        usize size() const;
        usize capacity() const;
        bool is_empty() const;
        bool is_long() const;

        void clear();
        void make_empty();
        void shrink(usize new_size);
        void grow(usize new_size, char c);
        void resize(usize new_size, char c = ' ');

        static String replaced(std::string_view str, std::string_view from, std::string_view to);
        String replaced(std::string_view from, std::string_view to) const;

        char* data();
        const char* data() const;

        iterator find(const char* str);
        const_iterator find(const char* str) const;

        iterator find(std::string_view str);
        const_iterator find(std::string_view str) const;

        std::string_view sub_str(usize beg) const;
        std::string_view sub_str(usize beg, usize len) const;

        bool starts_with(std::string_view str) const;
        bool ends_with(std::string_view str) const;

        explicit operator const char*() const;
        explicit operator char*();

        // to prevent Strings converting to bool via operator char*
        explicit operator bool() = delete;

        void swap(String& str);


        std::string_view view() const;
        operator std::string_view() const;

        void push_back(char c);

        String& operator+=(const String& str);
        String& operator+=(const char* str);
        String& operator+=(const std::string& str);
        String& operator+=(std::string_view str);
        // char deliberately excluded (causes problem when cat-ing numbers);

        char& operator[](usize i);
        char operator[](usize i) const;

        bool operator==(const char* str) const;
        bool operator!=(const char* str) const;

        bool operator==(const String& str) const;
        bool operator!=(const String& str) const;
        bool operator<(const String& str) const;

        bool operator==(std::string_view str) const;
        bool operator!=(std::string_view str) const;
        bool operator<(std::string_view str) const;

        // implemented in format.h
        template<typename T>
        core::String operator+(const T& r) const;

        template<typename T>
        String& operator=(T&& t) {
            return operator=(String(y_fwd(t)));
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

        String& append(const char* other_data, usize other_size);

        static char* alloc_long(usize capacity);
        static usize compute_capacity(usize len);
        static void free_long(LongData& d);
        static void free_short(ShortData& d);
        void set(const char* str, usize len);

        void free_data();

};


std::string_view trim_left(std::string_view str);
std::string_view trim_right(std::string_view str);
std::string_view trim(std::string_view str);


} // core

core::String operator+(std::string_view l, const core::String& r);

}


template<>
struct std::hash<y::core::String> : private std::hash<std::string_view> {
    auto operator()(const y::core::String& str) const {
        return std::hash<std::string_view>::operator()(str);
    }
};

#endif // Y_CORE_STRING_H

