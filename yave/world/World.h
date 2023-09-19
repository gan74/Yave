/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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
#ifndef YAVE_WORLD_WORLD_H
#define YAVE_WORLD_WORLD_H

#include "NodeRef.h"

#include <y/core/Vector.h>

namespace yave {

class NodeContainerBase : NonMovable {
    public:
        virtual ~NodeContainerBase();

        NodeType type() const;

    protected:
        NodeContainerBase(NodeType type);

        const NodeType _type;
};


template<typename T>
class NodeContainer : public NodeContainerBase {
    using Page = detail::Page<T>;
    using PagePtr = detail::PagePtr<T>;

    public:
        NodeContainer() : NodeContainerBase(node_type<T>()) {
        }

        template<typename... Args>
        NodeRef<T> add(Args&&... args) {
            NodeRef<T> ref = create_ref();
            ref._ptr->init(ref._generation, y_fwd(args)...);
            return ref;
        }

        void remove(NodeRef<T> ref) {
            y_debug_assert(!ref.is_null());
            y_debug_assert(std::any_of(_pages.begin(), _pages.end(), [&](const auto& page) { return page._ptr == detail::page_from_ptr<T>(ref._ptr); }));
            if(!ref._ptr->is_empty()) {
                ref._ptr->destroy();
                _free << ref;
            }
        }

    private:
        NodeRef<T> create_ref() {
            if(_free.is_empty()) {
                create_page();
                return create_ref();
            }

            NodeRef<T> ref = _free.pop();
            ++ref._generation;
            return ref;
        }

        void create_page() {
            PagePtr& page = _pages.emplace_back();
            page.init();
            for(usize i = 0; i != Page::element_count; ++i) {
                _free << NodeRef<T>(page.get(i));
            }
        }


        core::Vector<PagePtr> _pages;
        core::Vector<NodeRef<T>> _free;
};




class World {
    public:
        template<typename T, typename... Args>
        NodeRef<T> add(Args&&... args) {
            return create_container<T>().add(y_fwd(args)...);
        }

        template<typename T>
        void remove(NodeRef<T> ref) {
            create_container<T>().remove(ref);
        }

    private:
        template<typename T>
        NodeContainer<T>& create_container() {
            const NodeType type = node_type<T>();
            _containers.set_min_size(type + 1);
            auto& cont = _containers[type];
            if(!cont) {
                cont = std::make_unique<NodeContainer<T>>();
            }
            NodeContainer<T>* cont_ptr = dynamic_cast<NodeContainer<T>*>(cont.get());
            y_debug_assert(cont_ptr);
            return *cont_ptr;
        }


        core::Vector<std::unique_ptr<NodeContainerBase>> _containers;
};






}


#endif // YAVE_WORLD_WORLD_H

