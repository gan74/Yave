#ifndef Y_UTILS_RANGE_H
#define Y_UTILS_RANGE_H

#include <iterator>

namespace y {

template<typename B, typename E = B>
class Range {
	public:
		using iterator_traits = std::iterator_traits<B>;

		Range(const B& b, const E& e) : _begin(b), _end(e) {
		}

		B begin() const {
			return _begin;
		}

		E end() const {
			return _end;
		}

		B begin() {
			return _begin;
		}

		E end() {
			return _end;
		}

		bool operator==(const Range& other) const {
			return _begin == other._begin && _end == other._end;
		}

		bool operator!=(const Range& other) const {
			return !operator==(other);
		}

	private:
		B _begin;
		E _end;
};

template<typename B, typename E>
auto range(const B& b, const E& e) {
	return Range<B, E>(b, e);
}

}

#endif // Y_UTILS_RANGE_H
