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
#ifndef YAVE_BLUEPRINTS_LAMBDABLUEPRINTNODE_H
#define YAVE_BLUEPRINTS_LAMBDABLUEPRINTNODE_H

#include "BlueprintNodeFactory.h"

#include <y/utils/traits.h>
#include <y/core/String.h>
#include <y/core/Vector.h>

#include <array>
#include <tuple>

namespace yave {

namespace detail {
template<typename T>
static constexpr bool bp_is_input = !std::is_lvalue_reference_v<T> || std::is_const_v<std::remove_reference_t<T>>;

template<usize N, usize P>
constexpr std::array<usize, N> bp_port_indices(const std::array<bool, P>& is_input, bool input) {
    std::array<usize, N> indices = {};
    for(usize i = 0, j = 0; i != P; ++i) {
        if(is_input[i] == input) {
            indices[j++] = i;
        }
    }
    return indices;
}

template<typename F, typename Func = typename function_traits<F>::func_type>
class LambdaBlueprintNode;

template<typename F, typename Ret, typename... Args>
class LambdaBlueprintNode<F, Ret(Args...)> : public BlueprintNode {
    using values_t = std::tuple<std::remove_cvref_t<Args>...>;

    static constexpr usize port_count = sizeof...(Args);
    static constexpr std::array<bool, port_count> is_input = { bp_is_input<Args>... };
    static constexpr usize in_count = (0 + ... + usize(bp_is_input<Args>));
    static constexpr usize out_count = port_count - in_count;

    static constexpr auto input_indices = bp_port_indices<in_count>(is_input, true);
    static constexpr auto output_indices = bp_port_indices<out_count>(is_input, false);

    public:
        static std::shared_ptr<SharedBlueprintNodeData> make_shared_data(core::String name, core::Vector<core::String> names) {
            auto data = std::make_shared<SharedBlueprintNodeData>();
            data->name = std::move(name);
            for(usize i = 0; i != port_count; ++i) {
                (is_input[i] ? data->input_names : data->output_names).push_back(std::move(names[i]));
            }
            return data;
        }

        explicit LambdaBlueprintNode(std::shared_ptr<SharedBlueprintNodeData> shared_data) :
                BlueprintNode(std::move(shared_data)) {
        }

        usize input_count() const override {
            return in_count;
        }

        BlueprintParamTypeIndex input_type(usize index) const override {
            y_debug_assert(index < in_count);
            return _types[input_indices[index]];
        }

        void set_input(usize index, const void* ptr) override {
            y_debug_assert(index < in_count);
            _inputs[index] = ptr;
        }

        const void* input(usize index) const override {
            y_debug_assert(index < in_count);
            return _inputs[index];
        }

        void* default_input(usize index) override {
            y_debug_assert(index < in_count);
            return _value_ptrs[input_indices[index]];
        }

        usize output_count() const override {
            return out_count;
        }

        BlueprintParamTypeIndex output_type(usize index) const override {
            y_debug_assert(index < out_count);
            return _types[output_indices[index]];
        }

        const void* output_ptr(usize index) const override {
            y_debug_assert(index < out_count);
            return _value_ptrs[output_indices[index]];
        }

        void eval() override {
            // Connected inputs override the default values
            std::array<void*, port_count> ptrs = _value_ptrs;
            for(usize i = 0; i != in_count; ++i) {
                if(_inputs[i]) {
                    ptrs[input_indices[i]] = const_cast<void*>(_inputs[i]);
                }
            }

            [&]<usize... I>(std::index_sequence<I...>) {
                F{}(*static_cast<std::tuple_element_t<I, values_t>*>(ptrs[I])...);
            }(std::make_index_sequence<port_count>{});
        }

    private:
        static inline const std::array<BlueprintParamTypeIndex, port_count> _types = { blueprint_param_type_index<std::remove_cvref_t<Args>>()... };

        std::array<const void*, in_count> _inputs = {};

        values_t _values = {};
        const std::array<void*, port_count> _value_ptrs = std::apply([](auto&... values) { return std::array<void*, port_count>{ &values... }; }, _values);
};

template<typename F>
class LambdaBlueprintNodeFactory : public BlueprintNodeFactory {
    public:
        LambdaBlueprintNodeFactory(core::String name, core::Vector<core::String> names) :
                BlueprintNodeFactory(LambdaBlueprintNode<F>::make_shared_data(std::move(name), std::move(names))) {
        }

        std::unique_ptr<BlueprintNode> create_node() override {
            return std::make_unique<LambdaBlueprintNode<F>>(_shared_data);
        }
};

}

template<typename F, typename... Names>
std::unique_ptr<BlueprintNodeFactory> make_blueprint_node_factory(core::String name, const Names&... names) {
    static_assert(sizeof...(Names) == function_traits<F>::arg_count);
    return std::make_unique<detail::LambdaBlueprintNodeFactory<F>>(std::move(name), core::Vector<core::String>{core::String(names)...});
}

}

#endif // YAVE_BLUEPRINTS_LAMBDABLUEPRINTNODE_H
