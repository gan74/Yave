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
#ifndef YAVE_ECS_TAGS_H
#define YAVE_ECS_TAGS_H

#include <yave/yave.h>

#include <y/core/String.h>

namespace yave {
namespace ecs {

namespace tags {

#define DECLARE_TAG(tag)                                \
static const std::string_view tag = #tag;               \
static const std::string_view not_##tag = "!"#tag;



DECLARE_TAG(hidden)
DECLARE_TAG(selected)
DECLARE_TAG(debug)


#undef DECLARE_TAG
}

inline bool is_computed_tag(std::string_view tag) {
    return !tag.empty() && (tag[0] == '@' || tag[0] == '!');
}

inline bool is_computed_not_tag(std::string_view tag) {
    return !tag.empty() && tag[0] == '!';
}

inline std::string_view raw_tag(std::string_view tag) {
    return is_computed_tag(tag) ? tag.substr(1) : tag;
}

}
}


#endif // YAVE_ECS_TAGS_H

