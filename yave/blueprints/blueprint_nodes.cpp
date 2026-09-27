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
#include "LambdaBlueprintNode.h"

#include <y/math/Vec.h>
#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

template<typename T>
static void add_math_nodes(core::Vector<std::unique_ptr<BlueprintNodeFactory>>& factories, std::string_view type_name) {
    struct Const { void operator()(T&) const {} };
    factories.emplace_back(make_blueprint_node_factory<Const>(fmt_to_owned("Const {}", type_name), "value"));

    struct Negate { void operator()(T in, T& out) const { out = -in; } };
    factories.emplace_back(make_blueprint_node_factory<Negate>(fmt_to_owned("Negate {}", type_name), "in", "out"));

    struct Add { void operator()(T a, T b, T& out) const { out = a + b; } };
    factories.emplace_back(make_blueprint_node_factory<Add>(fmt_to_owned("Add {}", type_name), "a", "b", "out"));

    struct Multiply { void operator()(T a, T b, T& out) const { out = a * b; } };
    factories.emplace_back(make_blueprint_node_factory<Multiply>(fmt_to_owned("Multiply {}", type_name), "a", "b", "out"));

    struct Divide {
        void operator()(T a, T b, T& out) const {
            if(b == T(0)) {
                log_msg("Divide by 0", Log::Error);
                out = T(0);
            } else {
                out = a / b;
            }
        }
    };
    factories.emplace_back(make_blueprint_node_factory<Divide>(fmt_to_owned("Divide {}", type_name), "a", "b", "out"));
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
        factories.emplace_back(make_blueprint_node_factory<Create>(create_name, "out", "x"));
        factories.emplace_back(make_blueprint_node_factory<Decompose>(decomp_name, "in", "x"));
    } else if constexpr(N == 2) {
        struct Create { void operator()(V& out, T x, T y) const { out = V(x, y); } };
        struct Decompose { void operator()(V in, T& x, T& y) const { x = in[0]; y = in[1]; } };
        factories.emplace_back(make_blueprint_node_factory<Create>(create_name, "out", "x", "y"));
        factories.emplace_back(make_blueprint_node_factory<Decompose>(decomp_name, "in", "x", "y"));
    } else if constexpr(N == 3) {
        struct Create { void operator()(V& out, T x, T y, T z) const { out = V(x, y, z); } };
        struct Decompose { void operator()(V in, T& x, T& y, T& z) const { x = in[0]; y = in[1]; z = in[2]; } };
        factories.emplace_back(make_blueprint_node_factory<Create>(create_name, "out", "x", "y", "z"));
        factories.emplace_back(make_blueprint_node_factory<Decompose>(decomp_name, "in", "x", "y", "z"));
    } else {
        static_assert(N == 4);
        struct Create { void operator()(V& out, T x, T y, T z, T w) const { out = V(x, y, z, w); } };
        struct Decompose { void operator()(V in, T& x, T& y, T& z, T& w) const { x = in[0]; y = in[1]; z = in[2]; w = in[3]; } };
        factories.emplace_back(make_blueprint_node_factory<Create>(create_name, "out", "x", "y", "z", "w"));
        factories.emplace_back(make_blueprint_node_factory<Decompose>(decomp_name, "in", "x", "y", "z", "w"));
    }

    struct Dot { void operator()(V a, V b, T& out) const { out = a.dot(b); } };
    factories.emplace_back(make_blueprint_node_factory<Dot>(fmt_to_owned("Dot {}", type_name), "a", "b", "out"));

    struct Cross { void operator()(V a, V b, V& out) const { out = a.cross(b); } };
    factories.emplace_back(make_blueprint_node_factory<Cross>(fmt_to_owned("Cross {}", type_name), "a", "b", "out"));

    struct Normalize { void operator()(V in, V& out) const { out = in.normalized(); } };
    factories.emplace_back(make_blueprint_node_factory<Normalize>(fmt_to_owned("Normalize {}", type_name), "in", "out"));

    struct Length { void operator()(V in, T& out) const { out = T(in.length()); } };
    factories.emplace_back(make_blueprint_node_factory<Length>(fmt_to_owned("Length {}", type_name), "in", "out"));

    struct Abs { void operator()(V in, V& out) const { out = in.abs(); } };
    factories.emplace_back(make_blueprint_node_factory<Abs>(fmt_to_owned("Abs {}", type_name), "in", "out"));

    struct Saturate { void operator()(V in, V& out) const { out = in.saturated(); } };
    factories.emplace_back(make_blueprint_node_factory<Saturate>(fmt_to_owned("Saturate {}", type_name), "in", "out"));
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
