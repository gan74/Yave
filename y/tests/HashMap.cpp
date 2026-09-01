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

#include <y/test/test.h>

#include <y/core/Vector.h>
#include <y/core/HashMap.h>
#include <y/core/String.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <y/math/random.h>

#include <unordered_map>
#include <random>
#include <ctime>
#include <memory>

namespace {
using namespace y;
using namespace y::core;

template<typename K, typename V, typename H = Hash<K>>
using DefaultImpl = FlatHashMap<K, V, H>;

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
        Hash<T> hasher;
        usize h = hasher(k);
        return h % B;
    }
};

template<typename Map>
static core::Vector<std::pair<int, int>> to_vector(const Map& map) {
    auto vec = Vector<std::pair<int, int>>::with_capacity(map.size());
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

    auto keys = Vector<i32>::with_capacity(count);

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


y_test_func("HashMap basics") {
    static constexpr int max_key = 100000;
    DefaultImpl<int, int> map;

    for(int i = 0; i != max_key; ++i) {
        y_debug_assert(!map.contains(i));
        map.emplace(i, i * 2);
        y_debug_assert(map.contains(i));
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

    const auto m2 = fuzz<FlatHashMap<i32, i32>>(fuzz_count, seed);
    /*const auto m3 = fuzz<FlatHashMap<i32, i32>>(fuzz_count, seed);
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
        fmt_into(str, "{}", i);
        y_test_assert(map.insert({str, i}).second);

        for(int j = i; j >= 0; --j) {
            y_test_assert(map.find(fmt("{}", j))->second == j);
        }
    }

    y_test_assert(map.size() == max_key);

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
            y_test_assert(map.insert({i, RaiiCounter(&counter)}).second);
            y_test_assert(map.size() == usize(i) + 1);
        }

        y_test_assert(counter == 0);
    }

    y_test_assert(counter == max_key);
}

y_test_func("HashMap simple") {
    DefaultImpl<int, int> map;
     map.emplace(4, 13);
    y_test_assert(map.contains(4));
}

y_test_func("HashMap erase") {
    DefaultImpl<int, int> map;
    for(int i = 0; i != 100; ++i) {
        map.emplace(i, i * 3);
    }

    map.erase(50);
    y_test_assert(!map.contains(50));
    y_test_assert(map.size() == 99);
    y_test_assert(map.find(50) == map.end());

    for(int i = 0; i != 100; ++i) {
        if(i == 50) {
            continue;
        }
        y_test_assert(map.find(i)->second == i * 3);
    }

    map.erase(0);
    map.erase(99);
    y_test_assert(map.size() == 97);
    y_test_assert(!map.contains(0));
    y_test_assert(!map.contains(99));
}

y_test_func("HashMap operator index") {
    DefaultImpl<int, int> map;
    map[1] = 10;
    map[2] = 20;
    y_test_assert(map.size() == 2);
    y_test_assert(map[1] == 10);
    y_test_assert(map[2] == 20);

    map[1] = 11;
    y_test_assert(map[1] == 11);
    y_test_assert(map.size() == 2);

    y_test_assert(map[3] == 0);
    y_test_assert(map.size() == 3);
}

y_test_func("HashMap clear") {
    DefaultImpl<int, int> map;
    for(int i = 0; i != 50; ++i) {
        map.emplace(i, i);
    }

    map.make_empty();
    y_test_assert(map.is_empty());
    y_test_assert(map.size() == 0);
    y_test_assert(map.bucket_count() != 0);

    map.emplace(1, 2);
    y_test_assert(map.contains(1));

    map.clear();
    y_test_assert(map.is_empty());
    y_test_assert(map.bucket_count() == 0);
}

y_test_func("HashMap move") {
    DefaultImpl<int, int> a;
    for(int i = 0; i != 20; ++i) {
        a.emplace(i, i + 1);
    }

    DefaultImpl<int, int> b = std::move(a);
    y_test_assert(b.size() == 20);
    y_test_assert(b.find(7)->second == 8);

    DefaultImpl<int, int> c;
    c.emplace(0, 0);
    c = std::move(b);
    y_test_assert(c.size() == 20);
    y_test_assert(c.contains(19));
}

struct StatefulHash {
    usize salt = next_salt();

    static usize next_salt() {
        static usize s = 1;
        return s++;
    }

    template<typename T>
    usize operator()(const T& t) const {
        return Hash<T>{}(t) ^ (salt * 0x9e3779b97f4a7c15ull);
    }
};

y_test_func("HashMap swap stateful hasher") {
    FlatHashMap<int, int, StatefulHash> a;
    FlatHashMap<int, int, StatefulHash> b;

    for(int i = 0; i != 40; ++i) {
        a.emplace(i, i);
        b.emplace(i + 100, i + 100);
    }

    a.swap(b);

    // Lookups fail if Hasher bases were not swapped with the table.
    y_test_assert(a.size() == 40);
    y_test_assert(b.size() == 40);
    y_test_assert(a.contains(100));
    y_test_assert(b.contains(0));
    y_test_assert(!a.contains(0));
    y_test_assert(!b.contains(100));

    for(int i = 0; i != 40; ++i) {
        y_test_assert(a.find(i + 100)->second == i + 100);
        y_test_assert(b.find(i)->second == i);
    }
}

y_test_func("HashMap keys values") {
    DefaultImpl<int, int> map;
    map.emplace(1, 10);
    map.emplace(2, 20);
    map.emplace(3, 30);

    int key_sum = 0;
    int value_sum = 0;
    for(const int k : map.keys()) {
        key_sum += k;
    }
    for(const int v : map.values()) {
        value_sum += v;
    }
    y_test_assert(key_sum == 6);
    y_test_assert(value_sum == 60);
}

y_test_func("HashMap tombstone reinsert") {
    DefaultImpl<int, int> map;
    map.set_min_capacity(64);

    for(int i = 0; i != 80; ++i) {
        map.emplace(i, i);
    }
    y_test_assert(map.size() == 80);

    for(int i = 0; i != 80; i += 2) {
        map.erase(i);
    }
    y_test_assert(map.size() == 40);

    for(int i = 0; i != 80; ++i) {
        if(i % 2 == 0) {
            y_test_assert(!map.contains(i));
        } else {
            y_test_assert(map.find(i)->second == i);
        }
    }

    for(int i = 0; i != 80; i += 2) {
        y_test_assert(map.insert({i, i * 10}).second);
    }
    y_test_assert(map.size() == 80);

    for(int i = 0; i != 80; ++i) {
        const int expected = (i % 2 == 0) ? i * 10 : i;
        y_test_assert(map.find(i)->second == expected);
    }
}

y_test_func("HashMap erase all reinsert") {
    DefaultImpl<int, int, AbysmalHash> map;
    for(int i = 0; i != 100; ++i) {
        map.emplace(i, i);
    }
    for(int i = 0; i != 100; ++i) {
        map.erase(i);
    }
    y_test_assert(map.is_empty());

    for(int i = 0; i != 100; ++i) {
        y_test_assert(map.insert({i, -i}).second);
        y_test_assert(map.find(i)->second == -i);
    }
    y_test_assert(map.size() == 100);
}

y_test_func("HashMap unique ptr values") {
    DefaultImpl<int, std::unique_ptr<int>> map;
    for(int i = 0; i != 50; ++i) {
        y_test_assert(map.emplace(i, std::make_unique<int>(i * 2)).second);
    }
    y_test_assert(map.size() == 50);

    for(int i = 0; i != 50; ++i) {
        y_test_assert(*map.find(i)->second == i * 2);
    }

    map.erase(10);
    y_test_assert(!map.contains(10));
    y_test_assert(map.size() == 49);

    DefaultImpl<int, std::unique_ptr<int>> moved = std::move(map);
    y_test_assert(moved.size() == 49);
    y_test_assert(*moved.find(20)->second == 40);
}

y_test_func("HashMap string vector values") {
    DefaultImpl<String, Vector<int>> map;
    for(int g = 0; g != 20; ++g) {
        String key;
        fmt_into(key, "group-{}", g);
        Vector<int> values;
        for(int i = 0; i != 5; ++i) {
            values.push_back(g * 5 + i);
        }
        y_test_assert(map.insert({key, std::move(values)}).second);
    }

    y_test_assert(map.size() == 20);
    y_test_assert(map.find("group-3")->second.size() == 5);
    y_test_assert(map.find("group-3")->second[2] == 17);

    map.erase("group-3");
    y_test_assert(!map.contains("group-3"));

    auto& slot = map["group-3"];
    y_test_assert(slot.is_empty());
    slot.push_back(42);
    y_test_assert(map.find("group-3")->second[0] == 42);
}

y_test_func("HashMap iterate erase collect") {
    DefaultImpl<int, int> map;
    for(int i = 0; i != 200; ++i) {
        map.emplace(i, i);
    }

    Vector<int> to_erase;
    for(const auto& [k, v] : map) {
        if((k + v) % 3 == 0) {
            to_erase.push_back(k);
        }
    }
    for(int k : to_erase) {
        map.erase(k);
    }

    y_test_assert(map.size() == 200 - to_erase.size());
    for(int k : to_erase) {
        y_test_assert(!map.contains(k));
    }
    for(const auto& [k, v] : map) {
        y_test_assert(k == v);
        y_test_assert((k + v) % 3 != 0);
    }
}

y_test_func("HashMap reserve growth") {
    DefaultImpl<int, int> map;
    map.set_min_capacity(1000);
    const usize buckets = map.bucket_count();
    y_test_assert(buckets >= 1000);

    for(int i = 0; i != 800; ++i) {
        map.emplace(i, i);
    }
    y_test_assert(map.bucket_count() == buckets);
    y_test_assert(map.size() == 800);

    for(int i = 0; i != 800; ++i) {
        y_test_assert(map.find(i)->second == i);
    }
}

y_test_func("HashMap insert range") {
    Vector<std::pair<int, int>> pairs;
    for(int i = 0; i != 100; ++i) {
        pairs.push_back({i, i + 1000});
    }

    DefaultImpl<int, int> map;
    map.insert(pairs.begin(), pairs.end());
    y_test_assert(map.size() == 100);

    pairs.clear();
    for(int i = 50; i != 150; ++i) {
        pairs.push_back({i, -i});
    }
    map.insert(pairs.begin(), pairs.end());
    y_test_assert(map.size() == 150);
    y_test_assert(map.find(50)->second == 1050);
    y_test_assert(map.find(149)->second == -149);
}

y_test_func("HashMap collision stress") {
    FlatHashMap<int, int, BadHash<8>> map;
    for(int i = 0; i != 300; ++i) {
        y_test_assert(map.insert({i, i * i}).second);
    }
    y_test_assert(map.size() == 300);

    for(int i = 0; i != 300; i += 3) {
        map.erase(i);
    }
    for(int i = 0; i != 300; ++i) {
        if(i % 3 == 0) {
            y_test_assert(!map.contains(i));
        } else {
            y_test_assert(map.find(i)->second == i * i);
        }
    }

    for(int i = 0; i != 300; i += 3) {
        map[i] = -i;
    }
    for(int i = 0; i != 300; ++i) {
        const int expected = (i % 3 == 0) ? -i : i * i;
        y_test_assert(map.find(i)->second == expected);
    }
}

y_test_func("HashMap mirrored unordered") {
    const u32 seed = 0xC0FFEEu;
    math::FastRandom rng(seed);
    std::uniform_int_distribution<i32> dist;

    FlatHashMap<i32, i32> flat;
    std::unordered_map<i32, i32> ref;

    for(usize step = 0; step != 5000; ++step) {
        const i32 k = dist(rng);
        const i32 v = dist(rng);
        switch(rng() % 5) {
            case 0:
            case 1: {
                const auto [it, inserted] = flat.insert({k, v});
                const auto [rit, rinserted] = ref.insert({k, v});
                y_test_assert(inserted == rinserted);
                unused(it, rit);
            } break;

            case 2: {
                flat[k] = v;
                ref[k] = v;
            } break;

            case 3: {
                flat.erase(k);
                ref.erase(k);
            } break;

            default: {
                y_test_assert(flat.contains(k) == (ref.find(k) != ref.end()));
                if(flat.contains(k)) {
                    y_test_assert(flat.find(k)->second == ref.find(k)->second);
                }
            } break;
        }
        y_test_assert(flat.size() == ref.size());
    }

    y_test_assert(to_vector(flat) == to_vector(ref));
}

y_test_func("HashMap raii erase mix") {
    usize counter = 0;
    {
        DefaultImpl<int, RaiiCounter> map;
        for(int i = 0; i != 100; ++i) {
            map.insert({i, RaiiCounter(&counter)});
        }
        y_test_assert(counter == 0);

        for(int i = 0; i != 100; i += 2) {
            map.erase(i);
        }
        y_test_assert(counter == 50);

        map.make_empty();
        y_test_assert(counter == 100);

        for(int i = 0; i != 20; ++i) {
            map.insert({i, RaiiCounter(&counter)});
        }
        y_test_assert(counter == 100);
    }
    y_test_assert(counter == 120);
}

}

