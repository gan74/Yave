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

#include "BlueprintInstance.h"

#include <yave/assets/AssetTraits.h>

#include <y/reflect/reflect.h>

#include <memory>

namespace yave {

struct BlueprintLink {
    u32 src_node = 0;
    u32 src_pin = 0;
    u32 dst_node = 0;
    u32 dst_pin = 0;

    y_reflect(BlueprintLink, src_node, src_pin, dst_node, dst_pin)
};

class Blueprint {
    public:
        Blueprint() = default;

        core::Span<std::unique_ptr<BlueprintNode>> all_nodes() const;
        core::Span<BlueprintLink> links() const;

        usize find_node_index(const BlueprintNode* node) const;
        const BlueprintLink* find_link(usize dst_node, usize dst_pin) const;

        const BlueprintNode* add_node(std::unique_ptr<BlueprintNode> node);
        void remove_node(const BlueprintNode* node);

        void add_blueprint(Blueprint data);

        bool is_link_valid(const BlueprintNode* src, usize src_pin, const BlueprintNode* dst, usize dst_pin) const;
        void add_link(const BlueprintNode* src, usize src_pin, const BlueprintNode* dst, usize dst_pin);
        void remove_link(const BlueprintNode* dst, usize dst_pin);
        void clear_links();

        core::Result<BlueprintInstance, BlueprintError> create_instance() const;

        y_reflect(Blueprint, _nodes, _links)

    private:
        core::Vector<std::unique_ptr<BlueprintNode>> _nodes;
        core::Vector<BlueprintLink> _links;
};

YAVE_DECLARE_GENERIC_ASSET_TRAITS(Blueprint, AssetType::Blueprint);

}

#endif // YAVE_BLUEPRINTS_BLUEPRINT_H
