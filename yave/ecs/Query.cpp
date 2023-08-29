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

#include "Query.h"

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {
namespace ecs {

core::Vector<EntityId> QueryUtils::matching(core::Span<SetMatch> matches, core::Span<EntityId> ids) {
    y_profile();

    auto match = core::Vector<EntityId>::with_capacity(ids.size());
    for(EntityId id : ids) {
        bool matched = true;
        for(usize i = 0; matched && i != matches.size(); ++i) {
            matched &= matches[i].contains(id);
        }

        if(matched) {
            match.push_back(id);
        }
    }

    y_profile_msg(fmt_c_str("{} id matched", match.size()));

    return match;
}

}
}

