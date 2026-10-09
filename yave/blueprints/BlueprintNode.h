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
#include <y/core/Span.h>
#include <y/math/Vec.h>
#include <y/reflect/reflect.h>
#include <y/serde3/poly.h>

#include <string_view>

namespace yave {

struct BlueprintParamType {
    std::string_view name;
    u64 type_hash = 0;
    usize size = 0;
    usize alignment = 0;
};

template<typename T>
const BlueprintParamType* blueprint_param_type() {
    static_assert(!std::is_const_v<T> && !std::is_reference_v<T>);
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(std::is_trivially_destructible_v<T>);
    static const BlueprintParamType type = {
        ct_type_name<T>(),
        ct_type_hash<T>(),
        sizeof(T),
        alignof(T),
    };
    return &type;
}

struct BlueprintExec {
    bool active = false;
};

struct BlueprintPin {
    std::string_view name;
    const BlueprintParamType* type = nullptr;
    bool is_generic = false;

    bool is_exec() const {
        return type == blueprint_param_type<BlueprintExec>();
    }
};

inline bool are_blueprint_types_compatible(const BlueprintParamType* a, const BlueprintParamType* b) {
    return a && b ? a == b : a != b;
}

class BlueprintNode : NonMovable {
    public:
        virtual ~BlueprintNode();

        y_serde3_poly_abstract_base(BlueprintNode)

        const core::String& name() const;
        core::String& name();

        math::Vec2 position() const;
        void set_position(math::Vec2 pos);

        virtual std::string_view node_type_name() const = 0;

        virtual core::Span<BlueprintPin> input_pins() const;
        virtual core::Span<BlueprintPin> output_pins() const;
        virtual core::Span<BlueprintPin> param_pins() const;

        bool has_generic_pin() const;

        virtual void set_generic_type(const BlueprintParamType* type);
        virtual const BlueprintParamType* generic_type() const;

        virtual bool is_entry_point() const;

        virtual void* default_input(usize index);
        virtual void* param_ptr(usize index);

        virtual void compile(BlueprintCompiler& compiler) const = 0;

    protected:
        BlueprintNode() = default;
        BlueprintNode(core::String name);

        core::String _name;
        math::Vec2 _position;
};

}

#endif // YAVE_BLUEPRINTS_BLUEPRINT_NODE_H
