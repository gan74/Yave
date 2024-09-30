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
#ifndef YAVE_ECS_COMPONENTRUNTIMEINFO_H
#define YAVE_ECS_COMPONENTRUNTIMEINFO_H

#include "ecs.h"

#include <y/serde3/serde.h>
#include <y/utils/name.h>

#include <memory>
#include <cctype>

namespace yave {
namespace ecs {

template<typename T>
std::unique_ptr<ComponentContainerBase> create_container();

template<typename T>
void create_or_replace_component(EntityWorld& world, EntityId id);

template<typename T>
static core::Span<ComponentTypeIndex> required_component_types() {
    if constexpr(HasRequiredComponents<T>) {
        return T::required_component_types;
    }
    return {};
}


struct ComponentRuntimeInfo {
    ComponentTypeIndex type_id;
    std::string_view type_name;
    core::Span<ComponentTypeIndex> required;
    std::unique_ptr<ComponentContainerBase> (*create_type_container)() = nullptr;
    void (*add_or_replace_component)(EntityWorld&, EntityId) = nullptr;

    std::string_view clean_component_name() const {
        return clean_component_name(type_name);
    }

    template<typename T>
    static ComponentRuntimeInfo create() {
        return ComponentRuntimeInfo {
            type_index<T>(),
            ct_type_name<T>(),
            required_component_types<T>(),
            create_container<T>,
            create_or_replace_component<T>
        };
    }

    static std::string_view clean_component_name(std::string_view name) {
        if(name.empty()) {
            return "???";
        }

        if(name.starts_with("::")) {
            return clean_component_name(name.substr(2));
        }
        if(name.starts_with("class ")) {
            return clean_component_name(name.substr(6));
        }
        if(name.starts_with("struct ")) {
            return clean_component_name(name.substr(7));
        }
        if(const usize n = name.find("::"); n != std::string_view::npos) {
            if(std::all_of(name.data(), name.data() + n, [](char c) { return std::isalnum(int(c)); })) {
                return clean_component_name(name.substr(n));
            }
        }

        return name;
    }

    y_no_serde3()
};

}
}

#endif // YAVE_ECS_COMPONENTRUNTIMEINFO_H

