/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
#ifndef Y_CORE_RANGE_H
#define Y_CORE_RANGE_H

#include <y/utils.h>

#include <iterator>

namespace y {
namespace core {

template<typename Iter, typename EndIter = Iter>
class Range {
    public:
        using iterator_traits = std::iterator_traits<Iter>;

        using iterator = Iter;
        using const_iterator = Iter;

        //using value_type = typename iterator_traits::value_type;
        using value_type = std::remove_reference_t<decltype(*std::declval<Iter>())>;

        inline Range(const Range&) = default;
        inline Range& operator=(const Range&) = default;

        inline Range(Iter b, EndIter e) : _beg(b), _end(e) {
        }

        template<typename Coll>
        inline Range(const Coll& col) : Range(col.begin(), col.end()) {
        }

        template<typename Coll>
        inline Range(Coll& col) : Range(col.begin(), col.end()) {
        }

        inline Iter begin() const {
            return _beg;
        }

        inline EndIter end() const {
            return _end;
        }

        inline bool is_empty() const {
            return  _beg == _end;
        }

        inline usize size() const {
            static_assert(std::is_same_v<typename std::iterator_traits<Iter>::iterator_category, std::random_access_iterator_tag>);
            return _end - _beg;
        }

        inline decltype(auto) operator[](usize index) const {
            static_assert(std::is_same_v<typename std::iterator_traits<Iter>::iterator_category, std::random_access_iterator_tag>);
            y_debug_assert(index < size());
            return *(_beg + index);
        }

    private:
        Iter _beg;
        EndIter _end;
};

template<typename Coll>
Range(const Coll&) -> Range<typename Coll::const_iterator>;

template<typename Coll>
Range(Coll&) -> Range<typename Coll::iterator>;

}
}

#endif // Y_CORE_RANGE_H

