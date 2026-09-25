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
#ifndef YAVE_BLUEPRINTS_BLUEPRINT_NODE_H
#define YAVE_BLUEPRINTS_BLUEPRINT_NODE_H

#include <yave/yave.h>

#include <y/utils/traits.h>
#include <y/core/Span.h>
#include <y/core/Vector.h>

#include <array>
#include <memory>
#include <tuple>
#include <functional>
#include <string_view>
#include <algorithm>


namespace yave {

enum class BlueprintParamTypeIndex : u32 {
    invalid_index = u32(-1),
};

namespace detail {
BlueprintParamTypeIndex next_blueprint_param_type_index();
}

template<typename T>
BlueprintParamTypeIndex blueprint_param_type_index() {
    static_assert(!std::is_const_v<T> && !std::is_reference_v<T>);
    static BlueprintParamTypeIndex type = detail::next_blueprint_param_type_index();
    return type;
}




namespace detail {
template<typename T, usize N, usize... I>
auto make_ref_tuple_impl(const std::array<const void*, N>& a, std::index_sequence<I...>) {
    return std::tuple<const std::remove_reference_t<std::tuple_element_t<I, T>>&...>{
        *static_cast<const std::remove_reference_t<std::tuple_element_t<I, T>>*>(a[I])...
    };
}

template<typename T, usize N>
auto make_ref_tuple(const std::array<const void*, N>& a) {
    static_assert(std::tuple_size_v<T> == N);
    return make_ref_tuple_impl<T>(a, std::make_index_sequence<N>{});
}

template<typename T>
struct bp_in { using type = T; static constexpr bool is_input = true; };

template<typename T>
struct bp_out { using type = T; static constexpr bool is_input = false; };

template<typename Port>
using bp_maybe_in = std::conditional_t<Port::is_input, std::tuple<typename Port::type>, std::tuple<>>;

template<typename Port>
using bp_maybe_out = std::conditional_t<Port::is_input, std::tuple<>, std::tuple<typename Port::type>>;

template<typename... Ports>
using bp_inputs_t = decltype(std::tuple_cat(std::declval<bp_maybe_in<Ports>>()...));

template<typename... Ports>
using bp_outputs_t = decltype(std::tuple_cat(std::declval<bp_maybe_out<Ports>>()...));

template<usize I, usize J, typename... Ports>
struct bp_count_before;

template<usize I, usize J>
struct bp_count_before<I, J> {
    static constexpr usize inputs = 0;
    static constexpr usize outputs = 0;
};

template<usize I, usize J, typename P, typename... Rest>
struct bp_count_before<I, J, P, Rest...> {
    static constexpr usize inputs = (J < I && P::is_input ? 1 : 0) + bp_count_before<I, J + 1, Rest...>::inputs;
    static constexpr usize outputs = (J < I && !P::is_input ? 1 : 0) + bp_count_before<I, J + 1, Rest...>::outputs;
};

template<typename Tuple, usize... I>
std::array<const void*, sizeof...(I)> make_tuple_ptrs(Tuple& t, std::index_sequence<I...>) {
    return { static_cast<const void*>(&std::get<I>(t))... };
}

template<typename Tuple>
auto make_tuple_ptrs(Tuple& t) {
    return make_tuple_ptrs(t, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

template<usize I, typename... Ports, usize N, typename Outputs>
decltype(auto) bp_port_ref(const std::array<const void*, N>& inputs, Outputs& outputs) {
    using Port = std::tuple_element_t<I, std::tuple<Ports...>>;
    if constexpr(Port::is_input) {
        return *static_cast<const typename Port::type*>(inputs[bp_count_before<I, 0, Ports...>::inputs]);
    } else {
        return std::get<bp_count_before<I, 0, Ports...>::outputs>(outputs);
    }
}

template<typename... Ports, typename F, usize N, typename Outputs, usize... I>
void eval_bp_ports(F& func, const std::array<const void*, N>& inputs, Outputs& outputs, std::index_sequence<I...>) {
    std::apply(func, std::forward_as_tuple(bp_port_ref<I, Ports...>(inputs, outputs)...));
}

template<typename Tuple, usize... I>
std::array<BlueprintParamTypeIndex, sizeof...(I)> make_bp_types(std::index_sequence<I...>) {
    return { blueprint_param_type_index<std::tuple_element_t<I, Tuple>>()... };
}

template<typename Tuple>
auto make_bp_types() {
    return make_bp_types<Tuple>(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}
}



class BlueprintNode : NonMovable {
    public:
        virtual ~BlueprintNode();

        virtual usize input_count() const = 0;
        virtual BlueprintParamTypeIndex input_type(usize index) const = 0;
        virtual std::string_view input_name(usize index) const = 0;
        virtual void set_input(usize index, const void* ptr) = 0;

        virtual usize output_count() const = 0;
        virtual BlueprintParamTypeIndex output_type(usize index) const = 0;
        virtual std::string_view output_name(usize index) const = 0;
        virtual const void* output_ptr(usize index) const = 0;

        virtual void eval() = 0;

};


namespace detail {

template<typename F, typename... Ports>
class LambdaBlueprintNode : public BlueprintNode {
    public:
        static constexpr usize port_count = sizeof...(Ports);
        static constexpr usize in_count = (0 + ... + usize(Ports::is_input));
        static constexpr usize out_count = port_count - in_count;

        using inputs_t = bp_inputs_t<Ports...>;
        using outputs_t = bp_outputs_t<Ports...>;

        template<typename G>
        LambdaBlueprintNode(G&& func, core::Span<std::string_view> names) : _func(y_fwd(func)) {
            y_debug_assert(names.size() == port_count);

            if constexpr(port_count > 0) {
                usize in_i = 0;
                usize out_i = 0;
                usize i = 0;
                ((Ports::is_input
                    ? void(_input_names[in_i++] = names[i++])
                    : void(_output_names[out_i++] = names[i++])), ...);
            }
        }

        usize input_count() const override {
            return in_count;
        }

        BlueprintParamTypeIndex input_type(usize index) const override {
            return _input_types[index];
        }

        std::string_view input_name(usize index) const override {
            return _input_names[index];
        }

        void set_input(usize index, const void* ptr) override {
            _inputs[index] = ptr;
        }

        usize output_count() const override {
            return out_count;
        }

        BlueprintParamTypeIndex output_type(usize index) const override {
            return _output_types[index];
        }

        std::string_view output_name(usize index) const override {
            return _output_names[index];
        }

        const void* output_ptr(usize index) const override {
            return _output_ptrs[index];
        }

        void eval() override {
            y_debug_assert(std::all_of(_inputs.begin(), _inputs.end(), [](const void* p) { return p; }));
            eval_bp_ports<Ports...>(_func, _inputs, _outputs, std::make_index_sequence<port_count>{});
        }

    private:
        std::array<BlueprintParamTypeIndex, in_count> _input_types = make_bp_types<inputs_t>();
        std::array<BlueprintParamTypeIndex, out_count> _output_types = make_bp_types<outputs_t>();

        std::array<std::string_view, in_count> _input_names = {};
        std::array<std::string_view, out_count> _output_names = {};

        std::array<const void*, in_count> _inputs = {};
        outputs_t _outputs = {};
        std::array<const void*, out_count> _output_ptrs = make_tuple_ptrs(_outputs);

        F _func;
};

}

template<typename... Ports>
class LambdaBlueprintNodeBuilder {
    public:
        LambdaBlueprintNodeBuilder() = default;

        template<typename T>
        LambdaBlueprintNodeBuilder<Ports..., detail::bp_in<T>> add_input(std::string_view name) {
            core::Vector<std::string_view> names(_names);
            names.push_back(name);
            return LambdaBlueprintNodeBuilder<Ports..., detail::bp_in<T>>(std::move(names));
        }

        template<typename T>
        LambdaBlueprintNodeBuilder<Ports..., detail::bp_out<T>> add_output(std::string_view name) {
            core::Vector<std::string_view> names(_names);
            names.push_back(name);
            return LambdaBlueprintNodeBuilder<Ports..., detail::bp_out<T>>(std::move(names));
        }

        template<typename F>
        std::function<std::unique_ptr<BlueprintNode>()> build(F&& func) {
            static_assert(function_traits<std::remove_cvref_t<F>>::arg_count == sizeof...(Ports));
            return [f = y_fwd(func), names = std::move(_names)] { 
                return std::make_unique<detail::LambdaBlueprintNode<std::remove_cvref_t<F>, Ports...>>(f, names); 
            };
        }

    private:
        template<typename... P>
        friend class LambdaBlueprintNodeBuilder;

        LambdaBlueprintNodeBuilder(core::Vector<std::string_view> names) : _names(std::move(names)) {
        }

        core::Vector<std::string_view> _names;
};

}

#endif // YAVE_BLUEPRINTS_BLUEPRINT_NODE_H
