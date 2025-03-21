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

#include "AssetId.h"

#include <y/core/String.h>
#include <y/concurrent/SpinLock.h>

#include <y/utils/format.h>

#include <random>

namespace yave {


core::String stringify_id(AssetId id) {
    return fmt_to_owned("{:016x}", id.id());
}

core::Result<AssetId> parse_id(std::string_view text) {
    u64 uid = 0;
    if(std::from_chars(text.data(), text.data() + text.size(), uid, 16).ec != std::errc()) {
        return core::Err();
    }
    return core::Ok(AssetId::from_id(uid));
}

AssetId make_random_id() {
    static concurrent::SpinLock lock;
    static std::random_device r;
    static std::minstd_rand rng(r());
    static std::uniform_int_distribution<u64> distr;

    const auto _ = std::unique_lock(lock);
    return AssetId::from_id(distr(rng));
}

}


