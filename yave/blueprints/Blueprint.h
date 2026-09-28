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
#ifndef YAVE_BLUEPRINTS_BLUEPRINT_H
#define YAVE_BLUEPRINTS_BLUEPRINT_H

#include "BlueprintNode.h"

#include <memory>
#include <utility>

namespace yave {

struct BlueprintError {
    usize node_index = 0;
    core::String error;
};

class Blueprint : NonCopyable {
    public:
        Blueprint() = default;
        explicit Blueprint(BlueprintData data);

        core::Span<std::unique_ptr<BlueprintNode>> all_nodes() const;

        const BlueprintNode* add_node(std::unique_ptr<BlueprintNode> node);
        void remove_node(const BlueprintNode* node);

        std::pair<const BlueprintNode*, usize> find_output(const void* ptr) const;

        void clear_links();
        bool is_link_valid(const BlueprintNode* src, usize src_pin, const BlueprintNode* dst, usize dst_pin) const;
        void add_link(const BlueprintNode* src, usize src_pin, const BlueprintNode* dst, usize dst_pin);
        void remove_link(const BlueprintNode* dst, usize dst_pin);

        core::Result<void, BlueprintError> eval() noexcept;

    private:
        friend class BlueprintData;

        usize find_node_index(const BlueprintNode* node) const;
        static usize find_output_pin(const BlueprintNode& node, const void* ptr);

        void update_generic_types();

        core::Vector<std::unique_ptr<BlueprintNode>> _nodes;
};

}

#endif // YAVE_BLUEPRINTS_BLUEPRINT_H
