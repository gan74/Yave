/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#ifndef YAVE_YAVE_H
#define YAVE_YAVE_H

#include <yave/utils/forward.h>
#include <yave/utils/profile.h>

#include <y/math/Transform.h>

namespace y::core {
class String;
}

namespace yave {

using namespace y;

}

// Hack to get drawer from the editor
namespace editor {
using namespace yave;

struct DebugValues {
    bool bool_f[4] = {false, false, false, false};
    bool bool_t[4] = {true, true, true, true};
    float float_0[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float float_1[4] = {1.0f, 1.0f, 1.0f, 1.0f};
};


DebugValues& debug_values();
DirectDraw& debug_drawer();

}

#endif // YAVE_YAVE_H

