/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include "EventHandler.h"

#include <array>

namespace yave {

#define YAVE_KEYS(X_MACRO)              \
    X_MACRO(Key::Unknown)               \
    X_MACRO(Key::Tab)                   \
    X_MACRO(Key::Clear)                 \
    X_MACRO(Key::Backspace)             \
    X_MACRO(Key::Enter)                 \
    X_MACRO(Key::Escape)                \
    X_MACRO(Key::PageUp)                \
    X_MACRO(Key::PageDown)              \
    X_MACRO(Key::End)                   \
    X_MACRO(Key::Home)                  \
    X_MACRO(Key::Left)                  \
    X_MACRO(Key::Up)                    \
    X_MACRO(Key::Right)                 \
    X_MACRO(Key::Down)                  \
    X_MACRO(Key::Insert)                \
    X_MACRO(Key::Delete)                \
    X_MACRO(Key::Alt)                   \
    X_MACRO(Key::Ctrl)                  \
    X_MACRO(Key::F1)                    \
    X_MACRO(Key::F2)                    \
    X_MACRO(Key::F3)                    \
    X_MACRO(Key::F4)                    \
    X_MACRO(Key::F5)                    \
    X_MACRO(Key::F6)                    \
    X_MACRO(Key::F7)                    \
    X_MACRO(Key::F8)                    \
    X_MACRO(Key::F9)                    \
    X_MACRO(Key::F10_Reserved)          \
    X_MACRO(Key::F11)                   \
    X_MACRO(Key::F12)                   \
    X_MACRO(Key::Space)                 \
    X_MACRO(Key::A)                     \
    X_MACRO(Key::B)                     \
    X_MACRO(Key::C)                     \
    X_MACRO(Key::D)                     \
    X_MACRO(Key::E)                     \
    X_MACRO(Key::F)                     \
    X_MACRO(Key::G)                     \
    X_MACRO(Key::H)                     \
    X_MACRO(Key::I)                     \
    X_MACRO(Key::J)                     \
    X_MACRO(Key::K)                     \
    X_MACRO(Key::L)                     \
    X_MACRO(Key::M)                     \
    X_MACRO(Key::N)                     \
    X_MACRO(Key::O)                     \
    X_MACRO(Key::P)                     \
    X_MACRO(Key::Q)                     \
    X_MACRO(Key::R)                     \
    X_MACRO(Key::S)                     \
    X_MACRO(Key::T)                     \
    X_MACRO(Key::U)                     \
    X_MACRO(Key::V)                     \
    X_MACRO(Key::W)                     \
    X_MACRO(Key::X)                     \
    X_MACRO(Key::Y)                     \
    X_MACRO(Key::Z)


const char* key_name(Key key) {
    switch(key) {

#define CASE_KEY(key) case key: return (#key) + 5;
YAVE_KEYS(CASE_KEY)
#undef CASE_KEY

        default:
        break;
    }
    return "";
}

core::Span<Key> all_keys() {
    static const std::array keys = {

#define KEY(key) key,
YAVE_KEYS(KEY)
#undef KEY

    };
    return keys;
}

bool is_character_key(Key key) {
    return (u32(key) >= u32(Key::A) && u32(key) <= u32(Key::Z)) || key == Key::Space;
}




static constexpr u32 packed_key_index(Key key) {
    if(u32(key) < u32(Key::MaxNonChar)) {
       return u32(key);
    }
    if(key == Key::Space) {
        return u32(Key::MaxNonChar);
    }
    const u32 packed = u32(Key::MaxNonChar) + 1 + u32(key) - u32(Key::A);
    y_debug_assert(packed < 64);
    return packed;
}

static_assert(packed_key_index(Key::Max) < 64);



KeyCombination::KeyCombination(Key key) {
    operator+=(key);
}

KeyCombination& KeyCombination::operator+=(Key key) {
    _bits |= u64(1) << packed_key_index(key);
    return *this;
}

bool KeyCombination::contains(KeyCombination keys) const {
    return (_bits & keys._bits) == keys._bits;
}

bool KeyCombination::contains(Key key) const {
    return ((u64(1) << packed_key_index(key)) & _bits) != 0;
}

bool KeyCombination::is_empty() const {
    return !_bits;
}

KeyCombination operator+(Key a, Key b) {
    return KeyCombination(a) += b;
}

KeyCombination operator+(KeyCombination a, Key b) {
    return a += b;

}

#undef YAVE_KEYS

}


