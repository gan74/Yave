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
#include "BlueprintCompiler.h"
#include "TriggerBlueprintNode.h"

#include <yave/systems/JoltPhysicsSystem.h>
#include <yave/ecs/EntityWorld.h>

#include <y/utils/traits.h>
#include <y/core/String.h>
#include <y/core/Vector.h>
#include <y/core/FixedArray.h>
#include <y/math/Vec.h>
#include <y/utils/format.h>
#include <y/reflect/reflect.h>
#include <y/serde3/archives.h>
#include <y/serde3/poly.h>

#include <array>
#include <concepts>
#include <cstring>
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
    using args_t = std::tuple<std::remove_cvref_t<Args>...>;

    static constexpr usize port_count = sizeof...(Args);
    static_assert(sizeof...(Names) == port_count);

    static constexpr std::array<bool, port_count> is_input = { detail::bp_is_input<Args>... };
    static constexpr usize in_count = (0 + ... + usize(detail::bp_is_input<Args>));
    static constexpr usize out_count = port_count - in_count;

    static constexpr auto input_indices = detail::bp_port_indices<in_count>(is_input, true);
    static constexpr auto output_indices = detail::bp_port_indices<out_count>(is_input, false);

    using inputs_t = decltype([]<usize... I>(std::index_sequence<I...>) {
        return std::tuple<std::tuple_element_t<input_indices[I], args_t>...>{};
    }(std::make_index_sequence<in_count>{}));

    template<usize N>
    static std::array<BlueprintPin, N> make_pins(const std::array<usize, N>& indices) {
        const std::array<std::string_view, port_count> names = { std::string_view(Names)... };
        const std::array<const BlueprintParamType*, port_count> types = { blueprint_param_type<std::remove_cvref_t<Args>>()... };

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

        LambdaBlueprintNodeImpl(core::String name) : BlueprintNode(std::move(name)) {
        }

        std::string_view node_type_name() const override {
            return clean_type_name<F>();
        }

        core::Span<BlueprintPin> input_pins() const override {
            return static_input_pins;
        }

        core::Span<BlueprintPin> output_pins() const override {
            return static_output_pins;
        }

        void* default_input(usize index) override {
            y_debug_assert(index < in_count);
            return std::apply([&](auto&... values) { return std::array<void*, in_count>{&values...}[index]; }, _values);
        }

        void compile(BlueprintCompiler& compiler) const override {
            std::array<void*, port_count> ptrs = {};
            for(usize i = 0; i != in_count; ++i) {
                ptrs[input_indices[i]] = const_cast<void*>(compiler.input(i));
            }
            for(usize i = 0; i != out_count; ++i) {
                ptrs[output_indices[i]] = compiler.output(i);
            }

            compiler.emit([ptrs](const BlueprintContext&) {
                [&]<usize... I>(std::index_sequence<I...>) {
                    F{}(*static_cast<std::tuple_element_t<I, args_t>*>(ptrs[I])...);
                }(std::make_index_sequence<port_count>{});
            });
        }

        y_reflect(LambdaBlueprintNodeImpl, _name, _values)
        y_serde3_poly(LambdaBlueprintNodeImpl)

    private:
        inputs_t _values = {};
};

template<typename F, FixedString... Names>
using LambdaBlueprintNode = LambdaBlueprintNodeImpl<F, typename function_traits<F>::func_type, Names...>;

template<typename T>
class ConstantBlueprintNode : public BlueprintNode {
    static inline const BlueprintPin static_value_pin = { "value", blueprint_param_type<T>() };

    public:
        ConstantBlueprintNode() = default;

        ConstantBlueprintNode(core::String name) : BlueprintNode(std::move(name)) {
        }

        std::string_view node_type_name() const override {
            return "Const";
        }

        core::Span<BlueprintPin> output_pins() const override {
            return static_value_pin;
        }

        core::Span<BlueprintPin> param_pins() const override {
            return static_value_pin;
        }

        void* param_ptr(usize index) override {
            unused(index);
            y_debug_assert(index == 0);
            return &_value;
        }

        void compile(BlueprintCompiler& compiler) const override {
            compiler.bind_output(0, compiler.alloc<T>(_value));
        }

        y_reflect(ConstantBlueprintNode, _name, _value)
        y_serde3_poly(ConstantBlueprintNode)

    private:
        T _value = {};
};

class IfBlueprintNode : public BlueprintNode {
    public:
        IfBlueprintNode() = default;

        IfBlueprintNode(core::String name) : BlueprintNode(std::move(name)) {
        }

        std::string_view node_type_name() const override {
            return "If";
        }

        core::Span<BlueprintPin> input_pins() const override {
            return _in_pins;
        }

        core::Span<BlueprintPin> output_pins() const override {
            return _out_pin;
        }

        void set_generic_type(const BlueprintParamType* type) override {
            y_debug_assert(!_out_pin.type != !type);
            _in_pins[1].type = type;
            _in_pins[2].type = type;
            _out_pin.type = type;
        }

        const BlueprintParamType* generic_type() const override {
            return _out_pin.type;
        }

        void* default_input(usize index) override {
            y_debug_assert(index < _in_pins.size());
            return index ? nullptr : &_default_cond;
        }

        void compile(BlueprintCompiler& compiler) const override {
            const bool* condition = static_cast<const bool*>(compiler.input(0));
            const void* if_true = compiler.input(1);
            const void* if_false = compiler.input(2);
            void* out = compiler.output(0);
            const usize size = _out_pin.type->size;

            compiler.emit([=](const BlueprintContext&) {
                std::memcpy(out, *condition ? if_true : if_false, size);
            });
        }

        y_reflect(IfBlueprintNode, _name, _default_cond)
        y_serde3_poly(IfBlueprintNode)

    private:
        std::array<BlueprintPin, 3> _in_pins = {{{"condition", blueprint_param_type<bool>()}, {"true", nullptr, true}, {"false", nullptr, true}}};
        BlueprintPin _out_pin = {"out", nullptr, true};

        bool _default_cond = true;
};

class BranchBlueprintNode : public BlueprintNode {
    static inline const std::array<BlueprintPin, 2> static_input_pins = {{{"exec", blueprint_param_type<BlueprintExec>()}, {"condition", blueprint_param_type<bool>()}}};
    static inline const std::array<BlueprintPin, 2> static_output_pins = {{{"true", blueprint_param_type<BlueprintExec>()}, {"false", blueprint_param_type<BlueprintExec>()}}};

    public:
        BranchBlueprintNode() = default;

        BranchBlueprintNode(core::String name) : BlueprintNode(std::move(name)) {
        }

        std::string_view node_type_name() const override {
            return "Branch";
        }

        core::Span<BlueprintPin> input_pins() const override {
            return static_input_pins;
        }

        core::Span<BlueprintPin> output_pins() const override {
            return static_output_pins;
        }

        void* default_input(usize index) override {
            y_debug_assert(index < static_input_pins.size());
            return index ? &_default_cond : nullptr;
        }

        void compile(BlueprintCompiler& compiler) const override {
            const BlueprintExec* exec = static_cast<const BlueprintExec*>(compiler.input(0));
            const bool* condition = static_cast<const bool*>(compiler.input(1));
            BlueprintExec* if_true = static_cast<BlueprintExec*>(compiler.output(0));
            BlueprintExec* if_false = static_cast<BlueprintExec*>(compiler.output(1));

            compiler.emit([=](const BlueprintContext&) {
                if_true->active = exec->active && *condition;
                if_false->active = exec->active && !*condition;
            });
        }

        y_reflect(BranchBlueprintNode, _name, _default_cond)
        y_serde3_poly(BranchBlueprintNode)

    private:
        bool _default_cond = true;
};

class RemoveEntityBlueprintNode : public BlueprintNode {
    static inline const std::array<BlueprintPin, 2> static_input_pins = {{{"exec", blueprint_param_type<BlueprintExec>()}, {"entity", blueprint_param_type<ecs::EntityId>()}}};

    public:
        RemoveEntityBlueprintNode() = default;

        RemoveEntityBlueprintNode(core::String name) : BlueprintNode(std::move(name)) {
        }

        std::string_view node_type_name() const override {
            return "Remove entity";
        }

        core::Span<BlueprintPin> input_pins() const override {
            return static_input_pins;
        }

        void compile(BlueprintCompiler& compiler) const override {
            const BlueprintExec* exec = static_cast<const BlueprintExec*>(compiler.input(0));
            const ecs::EntityId* entity = static_cast<const ecs::EntityId*>(compiler.input(1));
            compiler.emit([=](const BlueprintContext& context) {
                if(!exec->active || !entity->is_valid()) {
                    return;
                }

                if(!context.world) {
                    throw std::runtime_error("No world");
                }

                if(context.world->exists(*entity)) {
                    context.world->remove_entity(*entity);
                }
            });
        }

        y_reflect(RemoveEntityBlueprintNode, _name)
        y_serde3_poly(RemoveEntityBlueprintNode)
};

class SelfBlueprintNode : public BlueprintNode {
    static inline const BlueprintPin static_output_pin = { "entity", blueprint_param_type<ecs::EntityId>() };

    public:
        SelfBlueprintNode() = default;

        SelfBlueprintNode(core::String name) : BlueprintNode(std::move(name)) {
        }

        std::string_view node_type_name() const override {
            return "Self";
        }

        core::Span<BlueprintPin> output_pins() const override {
            return static_output_pin;
        }

        void compile(BlueprintCompiler& compiler) const override {
            ecs::EntityId* out = static_cast<ecs::EntityId*>(compiler.output(0));
            compiler.emit([=](const BlueprintContext& context) {
                *out = context.self;
            });
        }

        y_reflect(SelfBlueprintNode, _name)
        y_serde3_poly(SelfBlueprintNode)
};




template<typename F, FixedString... Names>
static std::unique_ptr<BlueprintNodeFactory> make_blueprint_node_factory(core::String name) {
    static_assert(sizeof...(Names) == function_traits<F>::arg_count);
    return std::make_unique<GenericBlueprintNodeFactory<LambdaBlueprintNode<F, Names...>>>(std::move(name));
}



template<typename T>
static void add_arith_nodes(core::Vector<std::unique_ptr<BlueprintNodeFactory>>& factories, std::string_view type_name) {
    factories.emplace_back(std::make_unique<GenericBlueprintNodeFactory<ConstantBlueprintNode<T>>>(fmt_to_owned("Const {}", type_name)));

    struct Negate { void operator()(T in, T& out) const { out = -in; } };
    factories.emplace_back(make_blueprint_node_factory<Negate, "in", "out">(fmt_to_owned("Negate {}", type_name)));

    struct Add { void operator()(T a, T b, T& out) const { out = a + b; } };
    factories.emplace_back(make_blueprint_node_factory<Add, "a", "b", "out">(fmt_to_owned("Add {}", type_name)));

    struct Multiply { void operator()(T a, T b, T& out) const { out = a * b; } };
    factories.emplace_back(make_blueprint_node_factory<Multiply, "a", "b", "out">(fmt_to_owned("Multiply {}", type_name)));

    struct Divide {
        void operator()(T a, T b, T& out) const {
            if constexpr(is_iterable<T>) {
                for(const auto& elem : b) {
                    if(elem == std::remove_cvref_t<decltype(elem)>{}) {
                        throw std::runtime_error("Division by zero");
                    }
                }
            } else {
                if(b == T{}) {
                    throw std::runtime_error("Division by zero");
                }
            }

            out = a / b;
        }
    };
    factories.emplace_back(make_blueprint_node_factory<Divide, "a", "b", "out">(fmt_to_owned("Divide {}", type_name)));
}

template<typename T>
static void add_comp_nodes(core::Vector<std::unique_ptr<BlueprintNodeFactory>>& factories, std::string_view type_name) {
    struct Equal { void operator()(T a, T b, bool& out) const { out = a == b; } };
    factories.emplace_back(make_blueprint_node_factory<Equal, "a", "b", "out">(fmt_to_owned("Equal {}", type_name)));

    struct NotEqual { void operator()(T a, T b, bool& out) const { out = a != b; } };
    factories.emplace_back(make_blueprint_node_factory<NotEqual, "a", "b", "out">(fmt_to_owned("Not equal {}", type_name)));

    if constexpr(std::totally_ordered<T>) {
        struct Less { void operator()(T a, T b, bool& out) const { out = a < b; } };
        factories.emplace_back(make_blueprint_node_factory<Less, "a", "b", "out">(fmt_to_owned("Less {}", type_name)));

        struct Greater { void operator()(T a, T b, bool& out) const { out = a > b; } };
        factories.emplace_back(make_blueprint_node_factory<Greater, "a", "b", "out">(fmt_to_owned("Greater {}", type_name)));

        struct LessEqual { void operator()(T a, T b, bool& out) const { out = a <= b; } };
        factories.emplace_back(make_blueprint_node_factory<LessEqual, "a", "b", "out">(fmt_to_owned("Less or equal {}", type_name)));

        struct GreaterEqual { void operator()(T a, T b, bool& out) const { out = a >= b; } };
        factories.emplace_back(make_blueprint_node_factory<GreaterEqual, "a", "b", "out">(fmt_to_owned("Greater or equal {}", type_name)));
    }
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

static void add_bool_nodes(core::Vector<std::unique_ptr<BlueprintNodeFactory>>& factories) {
    factories.emplace_back(std::make_unique<GenericBlueprintNodeFactory<ConstantBlueprintNode<bool>>>("Const bool"));

    struct Not { void operator()(bool in, bool& out) const { out = !in; } };
    factories.emplace_back(make_blueprint_node_factory<Not, "in", "out">("Not"));

    struct And { void operator()(bool a, bool b, bool& out) const { out = a && b; } };
    factories.emplace_back(make_blueprint_node_factory<And, "a", "b", "out">("And"));

    struct Or { void operator()(bool a, bool b, bool& out) const { out = a || b; } };
    factories.emplace_back(make_blueprint_node_factory<Or, "a", "b", "out">("Or"));

    struct Xor { void operator()(bool a, bool b, bool& out) const { out = a != b; } };
    factories.emplace_back(make_blueprint_node_factory<Xor, "a", "b", "out">("Xor"));
}

void add_all_nodes(core::Vector<std::unique_ptr<BlueprintNodeFactory>>& factories) {
    factories.emplace_back(std::make_unique<GenericBlueprintNodeFactory<IfBlueprintNode>>("If"));

    factories.emplace_back(std::make_unique<GenericBlueprintNodeFactory<TriggerBlueprintNode<OnCollide>>>("On collide"));

    factories.emplace_back(std::make_unique<GenericBlueprintNodeFactory<BranchBlueprintNode>>("Branch"));
    factories.emplace_back(std::make_unique<GenericBlueprintNodeFactory<SelfBlueprintNode>>("Self"));
    factories.emplace_back(std::make_unique<GenericBlueprintNodeFactory<RemoveEntityBlueprintNode>>("Remove entity"));

#ifdef Y_DEBUG
    struct Debug { void operator()(float a) const { log_msg(fmt("Debug blueprint node: {}", a)); } };
    factories.emplace_back(make_blueprint_node_factory<Debug, "in">("Debug"));
#endif

    add_bool_nodes(factories);

    add_arith_nodes<float>(factories, "float");
    add_arith_nodes<math::Vec2>(factories, "Vec2");
    add_arith_nodes<math::Vec3>(factories, "Vec3");
    add_arith_nodes<math::Vec4>(factories, "Vec4");

    add_vec_nodes<math::Vec2>(factories, "Vec2");
    add_vec_nodes<math::Vec3>(factories, "Vec3");
    add_vec_nodes<math::Vec4>(factories, "Vec4");

    add_comp_nodes<float>(factories, "float");
    add_comp_nodes<math::Vec2>(factories, "Vec2");
    add_comp_nodes<math::Vec3>(factories, "Vec3");
    add_comp_nodes<math::Vec4>(factories, "Vec4");
}

}
