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

#include "BlueprintNode.h"

#include <yave/ecs/TriggerManager.h>

#include <cstring>
#include <tuple>

namespace yave {

class TriggerBlueprintNodeBase : public BlueprintNode {
    public:
        bool is_entry_point() const override {
            return true;
        }

        virtual ecs::TriggerTypeIndex trigger_type() const = 0;
        virtual void subscribe(ecs::TriggerSubscriber& subscriber) const = 0;
        virtual void set_payload(const void* payload) = 0;

    protected:
        TriggerBlueprintNodeBase() = default;

        TriggerBlueprintNodeBase(core::String name) : BlueprintNode(std::move(name)) {
        }
};

template<typename T>
class TriggerBlueprintNode final : public TriggerBlueprintNodeBase {
    static inline const auto static_output_pins = std::apply([](const auto&... members) {
        return std::array<BlueprintPin, sizeof...(members)>{
            BlueprintPin{members.name, blueprint_param_type<std::remove_cvref_t<decltype(members.get(std::declval<const T&>()))>>()}...
        };
    }, reflect::list_members<T>());

    public:
        TriggerBlueprintNode() = default;

        TriggerBlueprintNode(core::String name) : TriggerBlueprintNodeBase(std::move(name)) {
        }

        std::unique_ptr<BlueprintNode> clone() const override {
            return std::make_unique<TriggerBlueprintNode>(_name);
        }

        std::string_view node_type_name() const override {
            auto remove_prefix = [](std::string_view str, std::string_view pref) {
                return str.starts_with(pref) ? std::string_view(str.data() + pref.size(), str.size() - pref.size()) : str;
            };

            return remove_prefix(remove_prefix(remove_prefix(ct_type_name<T>(), "class "), "struct "), "yave::");
        }

        core::Span<BlueprintPin> output_pins() const override {
            return static_output_pins;
        }

        void eval() override {
        }

        const void* output_ptr(usize index) const override {
            y_debug_assert(index < static_output_pins.size());
            return std::apply([&](const auto&... members) {
                return std::array<const void*, sizeof...(members)>{&members.get(_payload)...};
            }, reflect::list_members<T>())[index];
        }

        ecs::TriggerTypeIndex trigger_type() const override {
            return ecs::trigger_index<T>();
        }

        void subscribe(ecs::TriggerSubscriber& subscriber) const override {
            subscriber.subscribe<T>();
        }

        void set_payload(const void* payload) override {
            std::memcpy(&_payload, payload, sizeof(T));
        }

        y_reflect(TriggerBlueprintNode, _name)
        y_serde3_poly(TriggerBlueprintNode)

    private:
        T _payload = {};
};

}

#endif // YAVE_BLUEPRINTS_TRIGGERBLUEPRINTNODE_H

