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

#include <y/core/String.h>
#include <y/core/Vector.h>

#include <memory>
#include <string_view>

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

class SharedBlueprintNodeData {
    public:
        core::String name;
        core::Vector<core::String> input_names;
        core::Vector<core::String> output_names;
};

class BlueprintNode : NonMovable {
    public:
        virtual ~BlueprintNode();

        std::string_view name() const;
        std::string_view input_name(usize index) const;
        std::string_view output_name(usize index) const;

        virtual void reset_inputs();

        virtual usize input_count() const = 0;
        virtual BlueprintParamTypeIndex input_type(usize index) const = 0;
        virtual void set_input(usize index, const void* ptr) = 0;
        virtual const void* input(usize index) const = 0;

        virtual usize output_count() const = 0;
        virtual BlueprintParamTypeIndex output_type(usize index) const = 0;
        virtual const void* output_ptr(usize index) const = 0;

        virtual void eval() = 0;

    protected:
        BlueprintNode(std::shared_ptr<SharedBlueprintNodeData> shared_data);

    private:
        std::shared_ptr<SharedBlueprintNodeData> _shared_data;
};

}

#endif // YAVE_BLUEPRINTS_BLUEPRINT_NODE_H
