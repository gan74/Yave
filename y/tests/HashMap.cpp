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

#include <y/test/test.h>

#include <y/core/Vector.h>
#include <y/core/HashMap.h>
#include <y/core/String.h>

#include <y/utils/format.h>

#include <y/math/random.h>

#include <unordered_map>
#include <random>
#include <ctime>

namespace {
using namespace y;
using namespace y::core;

template<typename K, typename V, typename H = std::hash<K>>
using DefaultImpl = ExternalHashMap<K, V, H>;

struct RaiiCounter : NonCopyable {
    RaiiCounter(usize* ptr) : counter(ptr) {
    }

    RaiiCounter(RaiiCounter&& raii) {
        std::swap(raii.counter, counter);
    }

    ~RaiiCounter() {
        if(counter) {
            ++(*counter);
        }
    }

    usize* counter = nullptr;
};


struct PassthroughHash {
    template<typename T>
    usize operator()(const T& t) const {
        return usize(t);
    }
};

struct AbysmalHash {
    template<typename T>
    usize operator()(const T&) const {
        return 0;
    }
};

template<usize B>
struct BadHash {
    template<typename T>
    usize operator()(const T& k) const {
        std::hash<T> hasher;
        usize h = hasher(k);
        return h % B;
    }
};

template<typename Map>
static core::Vector<std::pair<int, int>> to_vector(const Map& map) {
    auto vec = vector_with_capacity<std::pair<int, int>>(map.size());
    for(const auto& [k, v] : map) {
        vec << std::make_pair(k, v);
    }
    sort(vec.begin(), vec.end());
    return vec;
}

template<typename Map>
static Map fuzz(usize count, u32 seed) {
    Map map;
    math::FastRandom actions(seed);

    math::FastRandom values(seed + 1);
    std::uniform_int_distribution<i32> dist;

    auto keys = vector_with_capacity<i32>(count);

    for(usize i = 0; i != count; ++i) {
        switch(actions() % 4) {
            case 0: {
                if(keys.is_empty()) {
                    continue;
                }
                const usize key_index = values() % keys.size();
                const i32 k = keys[key_index];
                keys.erase_unordered(keys.begin() + key_index);

                map.erase(map.find(k));
            }
            break;

            case 1: {
                if(keys.is_empty()) {
                    continue;
                }
                const usize key_index = values() % keys.size();
                const i32 k = keys[key_index];

                (*map.find(k)).second = dist(values);
            }
            break;

            default: {
                const i32 k = dist(values);
                const i32 v = dist(values);
                if(map.insert({k, v}).second) {
                    keys << k;
                }
            }
            break;
        }
    }
    return map;
}


Y_TODO(Test with complex objects)

y_test_func("HashMap basics") {
    static constexpr int max_key = 1000;
    DefaultImpl<int, int> map;

    for(int i = 0; i != max_key; ++i) {
        map.emplace(i, i * 2);
    }

    y_debug_assert(map.contains(4));
    y_debug_assert(!map.contains(max_key + 1));
    y_debug_assert(map.find(max_key + 1) == map.end());

    for(int i = 0; i != max_key; ++i) {
        const auto it = map.find(i);
        y_test_assert(it != map.end());
        y_test_assert((*it).first == i);
        y_test_assert((*it).second == 2 * i);
    }
}

y_test_func("HashMap bad hash") {
    static constexpr int max_key = 500;
    DefaultImpl<int, int, AbysmalHash> map;

    for(int i = 0; i != max_key; ++i) {
        map.emplace(i, i * 2);
    }

    for(int i = 0; i != max_key; ++i) {
        const auto it = map.find(i);
        y_test_assert(it != map.end());
        y_test_assert((*it).first == i);
        y_test_assert((*it).second == 2 * i);
    }
}

y_test_func("HashMap fuzz") {
    const u32 seed = u32(std::time(nullptr));
    const usize fuzz_count = 25000;
    const auto m0 = fuzz<std::unordered_map<i32, i32>>(fuzz_count, seed);

    const auto m2 = fuzz<ExternalHashMap<i32, i32>>(fuzz_count, seed);
    /*const auto m3 = fuzz<ExternalHashMap<i32, i32>>(fuzz_count, seed);
    const auto m4 = fuzz<HashMap<i32, i32>>(fuzz_count, seed);*/

    y_test_assert(to_vector(m0) == to_vector(m2));
    /*y_test_assert(to_vector(m0) == to_vector(m3));
    y_test_assert(to_vector(m0) == to_vector(m4));*/
}


y_test_func("HashMap duplicates") {
    static constexpr int max_key = 1000;
    DefaultImpl<int, int> map;
    for(int i = 0; i != max_key; ++i) {
        map.emplace(i, i * 2);
    }

    y_test_assert((*map.find(7)).second == 14);
    (*map.find(7)).second = 13;
    y_test_assert((*map.find(7)).second == 13);
    y_test_assert(!map.emplace(7, 9999).second);
    y_test_assert((*map.find(7)).second == 13);
}


y_test_func("HashMap strings") {
    static constexpr int max_key = 1000;

    DefaultImpl<core::String, int> map;
    for(int i = 0; i != max_key; ++i) {
        core::String str;
        fmt_into(str, "%", i);
        map.insert({str, i});
    }

    map.erase(map.find("589"));

    y_test_assert(!map.insert({"14", 0}).second);

    y_test_assert(map.find("17")->second == 17);
    y_test_assert(map.find("99")->second == 99);
    y_test_assert(map.find("997")->second == 997);
    y_test_assert(map.find("589") == map.end());
}

y_test_func("HashMap value dtors") {
    static constexpr int max_key = 1000;

    usize counter = 0;
    {
        DefaultImpl<int, RaiiCounter> map;
        for(int i = 0; i != max_key; ++i) {
            map.insert({i, RaiiCounter(&counter)});
        }

        y_test_assert(counter == 0);
    }

    y_test_assert(counter == max_key);
}


}

