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
#ifndef YAVE_ECS_ECS_H
#define YAVE_ECS_ECS_H

#include <yave/yave.h>

#include <compare>

namespace yave {
namespace ecs {

enum class ComponentTypeIndex : u32 {};

namespace detail {
ComponentTypeIndex next_type_index();
}


template<typename T>
ComponentTypeIndex type_index() {
    static_assert(!std::is_const_v<T> && !std::is_reference_v<T>);
    static ComponentTypeIndex type(detail::next_type_index());
    return type;
}




class EntityId {
    public:
        explicit EntityId(u32 index = invalid_index, u32 version = 0) : _index(index), _version(version) {
        }

        void invalidate() {
            _index = invalid_index;
        }

        void make_valid(u32 index) {
            _index = index;
            ++_version;
        }

        u32 index() const {
            return _index;
        }

        u32 version() const {
            return _version;
        }

        bool is_valid() const {
            return _index != invalid_index;
        }

        u64 as_u64() const {
            return (u64(_index) << 32) | _version;
        }

        std::strong_ordering operator<=>(const EntityId& other) const {
            return as_u64() <=> other.as_u64();
        }

        bool operator==(const EntityId& other) const = default;
        bool operator!=(const EntityId& other) const = default;

    private:
        static constexpr u32 invalid_index = u32(-1);

        u32 _index = invalid_index;
        u32 _version = 0;
};



template<typename Component, typename... SystemTypes>
struct SystemLinkedComponent {
    // EntityWorld.inl
    static inline void register_component_type(System*);
};



}
}

template<>
struct std::hash<yave::ecs::EntityId> : std::hash<y::u64> {
    auto operator()(const yave::ecs::EntityId& id) const {
        return hash<y::u64>::operator()(id.as_u64());
    }
};

#endif // YAVE_ECS_ECS_H

