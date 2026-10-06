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
#ifndef YAVE_BLUEPRINTS_TRIGGERBLUEPRINTNODE_H
#define YAVE_BLUEPRINTS_TRIGGERBLUEPRINTNODE_H

#include "BlueprintCompiler.h"

#include <yave/ecs/TriggerManager.h>

#include <array>
#include <tuple>

namespace yave {

template<typename T>
class TriggerBlueprintNode final : public BlueprintNode {
    static inline const auto static_output_pins = std::apply([](const auto&... members) {
        return std::array<BlueprintPin, sizeof...(members)>{
            BlueprintPin{members.name, blueprint_param_type<std::remove_cvref_t<decltype(members.get(std::declval<const T&>()))>>()}...
        };
    }, reflect::list_members<T>());

    public:
        TriggerBlueprintNode() = default;

        TriggerBlueprintNode(core::String name) : BlueprintNode(std::move(name)) {
        }

        std::string_view node_type_name() const override {
            return detail::bp_type_name<T>();
        }

        core::Span<BlueprintPin> output_pins() const override {
            return static_output_pins;
        }

        bool is_entry_point() const override {
            return true;
        }

        void compile(BlueprintCompiler& compiler) const override {
            T* payload = compiler.alloc<T>();

            std::apply([&](const auto&... members) {
                usize index = 0;
                (compiler.bind_output(index++, &members.get(*payload)), ...);
            }, reflect::list_members<T>());

            compiler.set_entry_point(ecs::trigger_index<T>(), payload, sizeof(T), [](ecs::TriggerSubscriber& subscriber) { subscriber.subscribe<T>(); });
        }

        y_reflect(TriggerBlueprintNode, _name)
        y_serde3_poly(TriggerBlueprintNode)
};

}

#endif // YAVE_BLUEPRINTS_TRIGGERBLUEPRINTNODE_H

