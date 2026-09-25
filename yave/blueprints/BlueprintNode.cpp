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

#include "BlueprintNode.h"

namespace yave {

namespace detail {
BlueprintParamTypeIndex next_blueprint_param_type_index() {
    static std::atomic<std::underlying_type_t<BlueprintParamTypeIndex>> global_type_index = 0;
    return BlueprintParamTypeIndex(global_type_index++);
}
}


BlueprintNode::~BlueprintNode() {
}





template<typename F>
auto make_func_bp_node(F&& func) {
    return FuncBlueprintNode<std::remove_cvref_t<F>>(y_fwd(func));
}

void test_bp_compile() {
    auto test_func = [](int x, double y) -> double { return x * y; };

    int i = 5;
    double d = 7.0;

    auto factory = LambdaBlueprintNodeBuilder<>()
        .add_input<int>("x")
        .add_output<double>("out")
        .add_input<double>("y")
        .build([](int x, double& out, double y) { out = x * y; });

    auto c = factory();

    y_debug_assert(c->input_name(0) == "x");
    y_debug_assert(c->input_name(1) == "y");
    y_debug_assert(c->output_name(0) == "out");

    c->set_input(0, &i);
    c->set_input(1, &d);
    c->eval();

    y_debug_assert(static_cast<const double*>(c->output_ptr(0))[0] == i * d);
}

}

