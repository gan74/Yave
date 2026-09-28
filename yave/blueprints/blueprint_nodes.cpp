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

#include "blueprint_nodes.h"

#include <y/utils/traits.h>
#include <y/core/String.h>
#include <y/core/Vector.h>
#include <y/math/Vec.h>
#include <y/utils/format.h>
#include <y/reflect/reflect.h>
#include <y/serde3/archives.h>
#include <y/serde3/poly.h>

#include <array>
#include <stdexcept>
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
}


template<typename F, typename Func, FixedString... Names>
class LambdaBlueprintNodeImpl;

template<typename F, typename Ret, typename... Args, FixedString... Names>
class LambdaBlueprintNodeImpl<F, Ret(Args...), Names...> : public BlueprintNode {
    using values_t = std::tuple<std::remove_cvref_t<Args>...>;

    static constexpr usize port_count = sizeof...(Args);
    static_assert(sizeof...(Names) == port_count);

    static constexpr std::array<bool, port_count> is_input = { detail::bp_is_input<Args>... };
    static constexpr usize in_count = (0 + ... + usize(detail::bp_is_input<Args>));
    static constexpr usize out_count = port_count - in_count;

    static constexpr auto input_indices = detail::bp_port_indices<in_count>(is_input, true);
    static constexpr auto output_indices = detail::bp_port_indices<out_count>(is_input, false);

    template<usize N>
    static std::array<BlueprintPin, N> make_pins(const std::array<usize, N>& indices) {
        const std::array<std::string_view, port_count> names = { std::string_view(Names)... };
        const std::array<const BlueprintParamType*, port_count> types = { blueprint_param_type_index<std::remove_cvref_t<Args>>()... };

        std::array<BlueprintPin, N> pins = {};
        for(usize i = 0; i != N; ++i) {
            pins[i] = { names[indices[i]], types[indices[i]] };
        }
        return pins;
    }

    static inline const std::array<BlueprintPin, in_count> static_input_pins = make_pins(input_indices);
    static inline const std::array<BlueprintPin, out_count> static_output_pins = make_pins(output_indices);

    public:
        LambdaBlueprintNodeImpl() = default;

        LambdaBlueprintNodeImpl(core::String name) : _name(std::move(name)) {
        }

        std::string_view name() const override {
            return _name;
        }

        core::Span<BlueprintPin> input_pins() const override {
            return static_input_pins;
        }

        core::Span<BlueprintPin> output_pins() const override {
            return static_output_pins;
        }

        void eval() override {
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

        const void* output_ptr(usize index) const override {
            y_debug_assert(index < out_count);
            return _value_ptrs[output_indices[index]];
        }

        y_reflect(LambdaBlueprintNodeImpl, _name, _values)
        y_serde3_poly(LambdaBlueprintNodeImpl)

    private:
        core::String _name;

        std::array<const void*, in_count> _inputs = {};

        values_t _values = {};
        const std::array<void*, port_count> _value_ptrs = std::apply([](auto&... values) { return std::array<void*, port_count>{ &values... }; }, _values);
};

template<typename F, FixedString... Names>
using LambdaBlueprintNode = LambdaBlueprintNodeImpl<F, typename function_traits<F>::func_type, Names...>;

template<typename T>
class ConstantBlueprintNode : public BlueprintNode {
    static inline const BlueprintPin static_value_pin = { "value", blueprint_param_type_index<T>() };

    public:
        ConstantBlueprintNode() = default;

        ConstantBlueprintNode(core::String name) : _name(std::move(name)) {
        }

        std::string_view name() const override {
            return _name;
        }

        core::Span<BlueprintPin> output_pins() const override {
            return static_value_pin;
        }

        core::Span<BlueprintPin> param_pins() const override {
            return static_value_pin;
        }

        void eval() override {
        }

        const void* output_ptr(usize index) const override {
            y_debug_assert(index == 0);
            return &_value;
        }

        void* param_ptr(usize index) override {
            y_debug_assert(index == 0);
            return &_value;
        }

        y_reflect(ConstantBlueprintNode, _name, _value)
        y_serde3_poly(ConstantBlueprintNode)

    private:
        core::String _name;

        T _value = {};
};




template<typename F, FixedString... Names>
static std::unique_ptr<BlueprintNodeFactory> make_blueprint_node_factory(core::String name) {
    static_assert(sizeof...(Names) == function_traits<F>::arg_count);
    return std::make_unique<GenericBlueprintNodeFactory<LambdaBlueprintNode<F, Names...>>>(std::move(name));
}



template<typename T>
static void add_math_nodes(core::Vector<std::unique_ptr<BlueprintNodeFactory>>& factories, std::string_view type_name) {
    factories.emplace_back(std::make_unique<GenericBlueprintNodeFactory<ConstantBlueprintNode<T>>>(fmt_to_owned("Const {}", type_name)));

    struct Negate { void operator()(T in, T& out) const { out = -in; } };
    factories.emplace_back(make_blueprint_node_factory<Negate, "in", "out">(fmt_to_owned("Negate {}", type_name)));

    struct Add { void operator()(T a, T b, T& out) const { out = a + b; } };
    factories.emplace_back(make_blueprint_node_factory<Add, "a", "b", "out">(fmt_to_owned("Add {}", type_name)));

    struct Multiply { void operator()(T a, T b, T& out) const { out = a * b; } };
    factories.emplace_back(make_blueprint_node_factory<Multiply, "a", "b", "out">(fmt_to_owned("Multiply {}", type_name)));

    struct Divide {
        void operator()(T a, T b, T& out) const {
            if(b == T(0)) {
                throw std::runtime_error("Division by zero");
            } else {
                out = a / b;
            }
        }
    };
    factories.emplace_back(make_blueprint_node_factory<Divide, "a", "b", "out">(fmt_to_owned("Divide {}", type_name)));
}

template<typename V>
static void add_vec_nodes(core::Vector<std::unique_ptr<BlueprintNodeFactory>>& factories, std::string_view type_name) {
    using T = typename V::value_type;
    static constexpr usize N = V::size();

    const core::String create_name = fmt_to_owned("Create {}", type_name);
    const core::String decomp_name = fmt_to_owned("Decompose {}", type_name);
    if constexpr(N == 1) {
        struct Create { void operator()(V& out, T x) const { out = V(x); } };
        struct Decompose { void operator()(V in, T& x) const { x = in[0]; } };
        factories.emplace_back(make_blueprint_node_factory<Create, "out", "x">(create_name));
        factories.emplace_back(make_blueprint_node_factory<Decompose, "in", "x">(decomp_name));
    } else if constexpr(N == 2) {
        struct Create { void operator()(V& out, T x, T y) const { out = V(x, y); } };
        struct Decompose { void operator()(V in, T& x, T& y) const { x = in[0]; y = in[1]; } };
        factories.emplace_back(make_blueprint_node_factory<Create, "out", "x", "y">(create_name));
        factories.emplace_back(make_blueprint_node_factory<Decompose, "in", "x", "y">(decomp_name));
    } else if constexpr(N == 3) {
        struct Create { void operator()(V& out, T x, T y, T z) const { out = V(x, y, z); } };
        struct Decompose { void operator()(V in, T& x, T& y, T& z) const { x = in[0]; y = in[1]; z = in[2]; } };
        factories.emplace_back(make_blueprint_node_factory<Create, "out", "x", "y", "z">(create_name));
        factories.emplace_back(make_blueprint_node_factory<Decompose, "in", "x", "y", "z">(decomp_name));
    } else {
        static_assert(N == 4);
        struct Create { void operator()(V& out, T x, T y, T z, T w) const { out = V(x, y, z, w); } };
        struct Decompose { void operator()(V in, T& x, T& y, T& z, T& w) const { x = in[0]; y = in[1]; z = in[2]; w = in[3]; } };
        factories.emplace_back(make_blueprint_node_factory<Create, "out", "x", "y", "z", "w">(create_name));
        factories.emplace_back(make_blueprint_node_factory<Decompose, "in", "x", "y", "z", "w">(decomp_name));
    }

    struct Dot { void operator()(V a, V b, T& out) const { out = a.dot(b); } };
    factories.emplace_back(make_blueprint_node_factory<Dot, "a", "b", "out">(fmt_to_owned("Dot {}", type_name)));

    struct Cross { void operator()(V a, V b, V& out) const { out = a.cross(b); } };
    factories.emplace_back(make_blueprint_node_factory<Cross, "a", "b", "out">(fmt_to_owned("Cross {}", type_name)));

    struct Normalize { void operator()(V in, V& out) const { out = in.normalized(); } };
    factories.emplace_back(make_blueprint_node_factory<Normalize, "in", "out">(fmt_to_owned("Normalize {}", type_name)));

    struct Length { void operator()(V in, T& out) const { out = T(in.length()); } };
    factories.emplace_back(make_blueprint_node_factory<Length, "in", "out">(fmt_to_owned("Length {}", type_name)));

    struct Abs { void operator()(V in, V& out) const { out = in.abs(); } };
    factories.emplace_back(make_blueprint_node_factory<Abs, "in", "out">(fmt_to_owned("Abs {}", type_name)));

    struct Saturate { void operator()(V in, V& out) const { out = in.saturated(); } };
    factories.emplace_back(make_blueprint_node_factory<Saturate, "in", "out">(fmt_to_owned("Saturate {}", type_name)));
}

void add_all_nodes(core::Vector<std::unique_ptr<BlueprintNodeFactory>>& factories) {
    add_math_nodes<float>(factories, "float");
    add_math_nodes<math::Vec2>(factories, "Vec2");
    add_math_nodes<math::Vec3>(factories, "Vec3");
    add_math_nodes<math::Vec4>(factories, "Vec4");

    add_vec_nodes<math::Vec2>(factories, "Vec2");
    add_vec_nodes<math::Vec3>(factories, "Vec3");
    add_vec_nodes<math::Vec4>(factories, "Vec4");
}

}
