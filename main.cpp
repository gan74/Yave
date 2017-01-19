#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>

#include <y/utils.h>
#include <y/core/Ptr.h>

using namespace y;

/*template<typename It, typename Compare = std::less<typename std::iterator_traits<It>::value_type>>
inline void insertion_sort(It current, It last, Compare compare = Compare()) {
	???
}

template<typename It, typename T, typename Compare>
inline It get_partition(It first, It last, const T& pivot, Compare compare) {
	for(;; ++first) {
		while(compare(*first, pivot)) {
			++first;
		}
		--last;

		while(compare(pivot, *last)) {
			--last;
		}

		if(first >= last) {
			return first;
		}

		std::swap(*first, *last);
	}
}

template<typename T, typename Compare>
inline const T& median(const T& first, const T& mid, const T& last, Compare compare) {
	const T& v = compare(first, mid) ? mid : first;
	return compare(v, last) ? mid : last;
}

template<typename It, typename Compare = std::less<typename std::iterator_traits<It>::value_type>>
inline void quick_sort_impl(It first, It last, isize limit, Compare compare) {
	auto dist = last - first;
	while(dist > limit) {
		auto med = median(*first, *(first + dist / 2), *(last - 1), compare);
		It position = get_partition(first, last, med, compare);

		quick_sort_impl(position, last, limit, compare);

		last = position;
		dist = last - first;
	}
}

template<typename It, typename Compare = std::less<typename std::iterator_traits<It>::value_type>>
inline void quick_sort(It first, It last, Compare compare = Compare()) {
	if(first != last) {
		quick_sort_impl(first, last, 1, compare);
		insertion_sort(first, last, compare);
	}
}
*/

struct Test {
	Test() {
		std::cout << "ctor" << std::endl;
	}
	~Test() {
		std::cout << "dtor" << std::endl;
	}
};

template<typename T = void>
static int test() {
	return 4;
}

int main() {
{
	core::Rc<Test[]> ptr(new Test[5]);
}
	/*std::vector<int> vec;
	while(vec.size() != 100000) {
		vec.push_back(rand());
	}

	insertion_sort(vec.begin(), vec.end());

	std::cout << std::boolalpha << std::is_sorted(vec.begin(), vec.end());
	std::cout << std::endl;*/
	return 0;
}






