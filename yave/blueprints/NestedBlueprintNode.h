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
#ifndef YAVE_BLUEPRINTS_NESTEDBLUEPRINTNODE_H
#define YAVE_BLUEPRINTS_NESTEDBLUEPRINTNODE_H

#include "Blueprint.h"

#include <yave/assets/AssetPtr.h>

#include <y/core/Result.h>

namespace yave {

class NestedBlueprintNode final : public BlueprintNode {
    public:
        NestedBlueprintNode() = default;
        NestedBlueprintNode(core::String name, AssetPtr<Blueprint> blueprint);

        std::unique_ptr<BlueprintNode> clone() const override;

        std::string_view node_type_name() const override;

        core::Span<BlueprintPin> input_pins() const override;
        core::Span<BlueprintPin> output_pins() const override;

        void eval() override;

        void set_input(usize index, const void* ptr) override;
        const void* input(usize index) const override;
        void* default_input(usize index) override;

        const void* output_ptr(usize index) const override;

        const AssetPtr<Blueprint>& blueprint() const;
        void set_blueprint(AssetPtr<Blueprint> blueprint);

        y_reflect(NestedBlueprintNode, _name, _blueprint, _input_values)
        y_serde3_poly(NestedBlueprintNode)

    private:
        struct InputValue {
            core::String name;
            u64 type = 0;
            core::Vector<u8> value;

            y_reflect(InputValue, name, type, value)
        };

        AssetPtr<Blueprint> _blueprint;
        BlueprintInstance _instance;
        core::Result<void, BlueprintError> _result = core::Err(BlueprintError{0, core::String("Nested blueprint is not loaded")});

        core::FixedArray<InputValue> _input_values;

        core::FixedArray<ParamInBlueprintNode*> _params_in;
        core::FixedArray<ParamOutBlueprintNode*> _params_out;

        core::FixedArray<BlueprintPin> _input_pins;
        core::FixedArray<BlueprintPin> _output_pins;
        core::FixedArray<const void*> _inputs;
};

}

#endif // YAVE_BLUEPRINTS_NESTEDBLUEPRINTNODE_H
