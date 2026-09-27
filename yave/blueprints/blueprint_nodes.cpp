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
#include "BlueprintNodeBuilder.h"

#include <y/math/Vec.h>
#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

template<typename T>
static void add_math_nodes(core::Vector<std::unique_ptr<BlueprintNodeFactory>>& factories, std::string_view type_name) {
    factories.emplace_back(LambdaBlueprintNodeBuilder<>(fmt_to_owned("Const {}", type_name))
        .add_output<T>("value")
        .build([](T& value) { value = T(1); })
    );

    factories.emplace_back(LambdaBlueprintNodeBuilder<>(fmt_to_owned("Negate {}", type_name))
        .add_input<T>("in")
        .add_output<T>("out")
        .build([](T in, T& out) { out = -in; })
    );

    factories.emplace_back(LambdaBlueprintNodeBuilder<>(fmt_to_owned("Add {}", type_name))
        .add_input<T>("a")
        .add_input<T>("b")
        .add_output<T>("out")
        .build([](T a, T b, T& out) { out = a + b; })
    );

    factories.emplace_back(LambdaBlueprintNodeBuilder<>(fmt_to_owned("Multiply {}", type_name))
        .add_input<T>("a")
        .add_input<T>("b")
        .add_output<T>("out")
        .build([](T a, T b, T& out) { out = a * b; })
    );

    factories.emplace_back(LambdaBlueprintNodeBuilder<>(fmt_to_owned("Divide {}", type_name))
        .add_input<T>("a")
        .add_input<T>("b")
        .add_output<T>("out")
        .build([](T a, T b, T& out) {
            if(b == T(0)) {
                log_msg("Divide by 0", Log::Error);
                out = T(0);
            } else {
                out = a / b;
            }
        })
    );
}

template<typename V>
static void add_vec_nodes(core::Vector<std::unique_ptr<BlueprintNodeFactory>>& factories, std::string_view type_name) {
    using T = typename V::value_type;
    static constexpr usize N = V::size();

    auto create_1 = LambdaBlueprintNodeBuilder<>(fmt_to_owned("Create {}", type_name)).add_output<V>("out").add_input<T>("x");
    auto decomp_1 = LambdaBlueprintNodeBuilder<>(fmt_to_owned("Decompose {}", type_name)).add_input<V>("in").add_output<T>("x");
    if constexpr(N == 1) {
        factories.emplace_back(create_1.build([](V& out, T x) { out = V(x); }));
        factories.emplace_back(decomp_1.build([](V out, T& x) { x = out[0]; }));
    } else {
        auto create_2 = create_1.add_input<T>("y");
        auto decomp_2 = decomp_1.add_output<T>("y");
        if constexpr(N == 2) {
            factories.emplace_back(create_2.build([](V& out, T x, T y) { out = V(x, y); }));
            factories.emplace_back(decomp_2.build([](V out, T& x, T& y) { x = out[0]; y = out[1]; }));
        } else {
            auto create_3 = create_2.add_input<T>("z");
            auto decomp_3 = decomp_2.add_output<T>("z");
            if constexpr(N == 3) {
                factories.emplace_back(create_3.build([](V& out, T x, T y, T z) { out = V(x, y, z); }));
                factories.emplace_back(decomp_3.build([](V out, T& x, T& y, T& z) { x = out[0]; y = out[1]; z = out[2]; }));
            } else {
                static_assert(N == 4);
                auto create_4 = create_3.add_input<T>("w");
                auto decomp_4 = decomp_3.add_output<T>("w");
                factories.emplace_back(create_4.build([](V& out, T x, T y, T z, T w) { out = V(x, y, z, w); }));
                factories.emplace_back(decomp_4.build([](V out, T& x, T& y, T& z, T& w) { x = out[0]; y = out[1]; z = out[2]; w = out[3]; }));
            }
        }
    }

    factories.emplace_back(LambdaBlueprintNodeBuilder<>(fmt_to_owned("Dot {}", type_name))
        .add_input<V>("a")
        .add_input<V>("b")
        .add_output<T>("out")
        .build([](V a, V b, T& out) { out = a.dot(b); })
    );

    factories.emplace_back(LambdaBlueprintNodeBuilder<>(fmt_to_owned("Cross {}", type_name))
        .add_input<V>("a")
        .add_input<V>("b")
        .add_output<V>("out")
        .build([](V a, V b, V& out) { out = a.cross(b); })
    );

    factories.emplace_back(LambdaBlueprintNodeBuilder<>(fmt_to_owned("Normalize {}", type_name))
        .add_input<V>("in")
        .add_output<V>("out")
        .build([](V in, V& out) { out = in.normalized(); })
    );

    factories.emplace_back(LambdaBlueprintNodeBuilder<>(fmt_to_owned("Length {}", type_name))
        .add_input<V>("in")
        .add_output<T>("out")
        .build([](V in, T& out) { out = T(in.length()); })
    );

    factories.emplace_back(LambdaBlueprintNodeBuilder<>(fmt_to_owned("Abs {}", type_name))
        .add_input<V>("in")
        .add_output<V>("out")
        .build([](V in, V& out) { out = in.abs(); })
    );

    factories.emplace_back(LambdaBlueprintNodeBuilder<>(fmt_to_owned("Saturate {}", type_name))
        .add_input<V>("in")
        .add_output<V>("out")
        .build([](V in, V& out) { out = in.saturated(); })
    );
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
