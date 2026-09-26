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

namespace yave {

class Blueprint : NonCopyable {
    public:
        const core::Span<std::unique_ptr<BlueprintNode>> all_nodes() const;

        const BlueprintNode* add_node(std::unique_ptr<BlueprintNode> node);

        void clear_links();
        bool is_link_valid(const BlueprintNode* src, usize src_pin, const BlueprintNode* dst, usize dst_pin) const;
        void add_link(const BlueprintNode* src, usize src_pin, const BlueprintNode* dst, usize dst_pin);

    private:
        core::Vector<std::unique_ptr<BlueprintNode>> _nodes;
};

}

#endif // YAVE_BLUEPRINTS_BLUEPRINT_H
