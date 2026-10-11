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
#ifndef YAVE_BLUEPRINTS_BLUEPRINTCOMPILER_H
#define YAVE_BLUEPRINTS_BLUEPRINTCOMPILER_H

#include "BlueprintInstance.h"

#include <memory>
#include <new>

namespace yave {

class BlueprintCompiler : NonMovable {
    public:
        const void* input(usize index);

        void* output(usize index);
        void bind_output(usize index, const void* ptr);

        void emit(std::function<void(const BlueprintContext&)> func);

        void set_entry_point(ecs::TriggerTypeIndex type, void* payload, usize payload_size, void (*subscribe)(ecs::TriggerSubscriber&));

        void error(core::String error);

        template<typename T>
        T* alloc(const T& value = {}) {
            static_assert(std::is_trivially_destructible_v<T>);
            return new(alloc(sizeof(T), alignof(T))) T(value);
        }

    private:
        friend class Blueprint;

        BlueprintCompiler(BlueprintStorage& storage);

        core::Result<void, BlueprintError> compile(core::Span<std::unique_ptr<BlueprintNode>> nodes, core::Span<BlueprintLink> links);

        BlueprintNode* current_node() const;

        void* alloc(usize size, usize alignment);
        void* alloc_copy(const BlueprintParamType* type, const void* value);

        BlueprintStorage& _storage;

        core::Span<std::unique_ptr<BlueprintNode>> _nodes;
        core::Span<BlueprintLink> _links;
        usize _current = 0;

        core::FixedArray<core::FixedArray<const void*>> _outputs;
        core::FixedArray<core::Vector<BlueprintInstruction>> _instructions;
        core::Vector<BlueprintInstance::EntryPoint> _entry_points;

        core::String _error;
};

}

#endif // YAVE_BLUEPRINTS_BLUEPRINTCOMPILER_H
