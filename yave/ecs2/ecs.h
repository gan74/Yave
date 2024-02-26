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
#ifndef YAVE_ECS2_ECS2_H
#define YAVE_ECS2_ECS2_H

#include <yave/yave.h>

#include <yave/ecs/ecs.h>

namespace yave {
namespace ecs2 {

using EntityId = ecs::EntityId;

enum class ComponentTypeIndex : u32 {
    invalid_index = u32(-1),
};

namespace detail {
ComponentTypeIndex next_type_index();
}


template<typename T>
ComponentTypeIndex type_index() {
    static_assert(!std::is_const_v<T> && !std::is_reference_v<T>);
    static ComponentTypeIndex type(detail::next_type_index());
    return type;
}

}
}


#endif // YAVE_ECS2_ECS_H

