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
#ifndef YAVE_WORLD_COMPONENTTYPE_H
#define YAVE_WORLD_COMPONENTTYPE_H

#include <yave/yave.h>

#include <y/core/Vector.h>

#include <array>
#include <bit>

namespace yave {

struct ComponentType {
    public:
        ComponentType() = default;

        u32 index() const {
            return _index;
        }

        std::string_view name() const {
            return _name;
        }

        inline bool operator==(const ComponentType& other) const {
            return _index == other._index;
        }

        inline bool operator!=(const ComponentType& other) const {
            return operator==(other);
        }

        inline bool operator<(const ComponentType& other) const {
            return _index < other._index;
        }

    private:
        template<typename T>
        friend ComponentType component_type();

        ComponentType(u32 i, std::string_view n) : _index(i), _name(n) {
        }

        u32 _index = 0;
        std::string_view _name;
};

namespace detail {
u32 create_component_type_index();
}

template<typename T>
ComponentType component_type() {
    static ComponentType type(detail::create_component_type_index(), ct_type_name<T>());
    return type;
}


template<typename T>
class ComponentContainer;


}


#endif // YAVE_WORLD_COMPONENTTYPE_H

