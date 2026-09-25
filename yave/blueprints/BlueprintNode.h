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

namespace yave {

enum class BlueprintParamTypeIndex : u32 {
    invalid_index = u32(-1),
};

namespace detail {
BlueprintParamTypeIndex next_blueprint_param_type_index();

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
}


template<typename T>
BlueprintParamTypeIndex blueprint_param_type_index() {
    static_assert(!std::is_const_v<T> && !std::is_reference_v<T>);
    static BlueprintParamTypeIndex type = detail::next_blueprint_param_type_index();
    return type;
}

class BlueprintNode : NonMovable {
    public:
        virtual ~BlueprintNode();

        virtual usize input_count() const = 0;
        virtual BlueprintParamTypeIndex input_type(usize index) const = 0;
        virtual void set_input(usize index, const void* ptr) = 0;

        virtual usize output_count() const = 0;
        virtual BlueprintParamTypeIndex output_type(usize index) const = 0; 
        virtual const void* output_ptr(usize index) const = 0;

        virtual void eval() = 0;

};

template<typename F>
class FuncBlueprintNode : public BlueprintNode {
    public:
        using traits = function_traits<F>;

        template<typename G>
        FuncBlueprintNode(G&& func) : _func(y_fwd(func)) {
        }

        usize input_count() const override {
            return traits::arg_count;
        }

        BlueprintParamTypeIndex input_type(usize index) const override {
            return _input_types[index];
        }

        void set_input(usize index, const void* ptr) override {
            _inputs[index] = ptr;
        }

        usize output_count() const override {
            return 1;
        }

        BlueprintParamTypeIndex output_type(usize) const override {
            return blueprint_param_type_index<traits::return_type>();
        }

        const void* output_ptr(usize) const override {
            return &_output;
        }

        void eval() override {
            y_debug_assert(std::all_of(_inputs.begin(), _inputs.end(), [](const void* p) { return p; }));
            auto args = detail::make_ref_tuple<traits::argument_pack>(_inputs);
            _output = std::apply(_func, args);
        }

    private:
        std::array<BlueprintParamTypeIndex, traits::arg_count> _input_types = []<usize... I>(std::index_sequence<I...>) { 
            return std::array{blueprint_param_type_index<std::tuple_element_t<I, traits::argument_pack>>()... };
        }(std::make_index_sequence<traits::arg_count>{});

        std::array<const void*, traits::arg_count> _inputs = {};
        traits::return_type _output = {};

        F _func;

};

}

#endif // YAVE_BLUEPRINTS_BLUEPRINT_NODE_H

