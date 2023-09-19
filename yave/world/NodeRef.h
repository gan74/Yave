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
#ifndef YAVE_WORLD_NODEREF_H
#define YAVE_WORLD_NODEREF_H

#include "NodeType.h"

#include <array>
#include <bit>

namespace yave {

using NodeType = u32;


template<typename T>
struct NodeElement : NonMovable {
    ~NodeElement() {
        if(!is_empty()) {
            destroy();
        }
    }

    T* get() {
        y_debug_assert(!is_empty());
        return &_node.node;
    }

    bool is_empty() const {
        return !_generation;
    }

    template<typename... Args>
    void init(u32 generation, Args&&... args) {
        y_debug_assert(is_empty());
        new(&_node.node) T(y_fwd(args)...);
        _generation = generation;
        y_debug_assert(!is_empty());
    }

    void destroy() {
        y_debug_assert(!is_empty());

        _generation = 0;
        _node.node.~T();

#ifdef Y_DEBUG
        std::memset(&_node.node, 0xFE, sizeof(_node.node));
#endif
    }

    union Node {
        Node() {
#ifdef Y_DEBUG
            std::memset(&node, 0xFE, sizeof(node));
#endif
        }

        T node;
    } _node;

    u32 _generation = 0;
};




namespace detail {

void* alloc_page(usize size, usize align);
void dealloc_page(void* ptr, usize size);


static constexpr usize node_page_size = 16 * 1024;
static constexpr usize node_page_alignement = node_page_size;


struct PageHeader {
    const NodeType type;

#ifdef Y_DEBUG
    static constexpr u32 fence_value = 0x12345678;
    u32 fence = fence_value;
#endif
};

template<typename T>
class PagePtr;

template<typename T>
class Page {
    public:
        static constexpr usize element_count = (node_page_size - sizeof(PageHeader)) / sizeof(NodeElement<T>);
        static_assert(element_count >= 1);

        detail::PageHeader header;
        std::array<NodeElement<T>, element_count> nodes;

    private:
        friend PagePtr<T>;

        Page() : header(node_type<T>()) {
            static_assert(offsetof(Page, header) == 0);
        }
};

template<typename T>
class PagePtr : NonCopyable {
    public:
        static_assert(sizeof(Page<T>) <= detail::node_page_size);

        PagePtr() = default;

        ~PagePtr() {
            if(_ptr) {
                _ptr->~Page();
                detail::dealloc_page(_ptr, sizeof(Page<T>));
            }
        }

        PagePtr(PagePtr&& other) {
            std::swap(_ptr, other._ptr);
        }

        PagePtr& operator=(PagePtr&& other) {
            std::swap(_ptr, other._ptr);
            return *this;
        }

        void init() {
            y_debug_assert(!_ptr);
            void* alloc = detail::alloc_page(sizeof(Page<T>), detail::node_page_alignement);
            _ptr = new(alloc) Page<T>();
        }

        NodeElement<T>* get(usize index) {
            y_debug_assert(_ptr);
            return &_ptr->nodes[index];
        }

    private:
        friend NodeContainer<T>;

        Page<T>* _ptr = nullptr;
};


inline PageHeader* page_header_from_ptr(void* ptr) {
    static_assert(sizeof(usize) == sizeof(ptr));
    const usize uptr = std::bit_cast<usize>(ptr);
    const usize start = uptr - (uptr % node_page_alignement);
    PageHeader* header = std::bit_cast<PageHeader*>(start);
    y_debug_assert(header->fence == PageHeader::fence_value);
    return header;
}

template<typename T>
inline Page<T>* page_from_ptr(void* ptr) {
    PageHeader* header = page_header_from_ptr(ptr);
    y_debug_assert(header->type == node_type<T>());
    return reinterpret_cast<Page<T>*>(header);
}

}








template<typename T>
struct NodeRef {
    public:
        NodeRef() = default;

        bool is_null() const {
            y_debug_assert(!_ptr == !_generation);
            return !_ptr;
        }

        const T* get() const {
            if(!_ptr) {
                return nullptr;
            }
            return _ptr->_generation == _generation ? _ptr->get() : nullptr;
        }

    private:
        friend NodeContainer<T>;
        friend class UntypedNodeRef;

        NodeRef(NodeElement<T>* ptr) : _ptr(ptr) {
        }

        NodeElement<T>* _ptr = nullptr;
        u32 _generation = 0;
};

class UntypedNodeRef {
    public:
        UntypedNodeRef() = default;

        template<typename T>
        UntypedNodeRef(NodeRef<T> ref) : _ptr(ref._ptr), _generation(ref._generation) {
            y_debug_assert(is<T>());
        }

        template<typename T>
        bool is() const {
            return _ptr ? detail::page_header_from_ptr(_ptr)->type == node_type<T>() : false;
        }

        template<typename T>
        NodeRef<T> to_typed() const {
            if(!is<T>()) {
            }

            NodeElement<T>* element = reinterpret_cast<NodeElement<T>*>(_ptr);
            if(element->_generation != _generation) {
                return {};
            }

            NodeRef<T> ref;
            ref._ptr = element;
            ref._generation = _generation;
            return ref;
        }

    private:
        void* _ptr = nullptr;
        u32 _generation = 0;
};

}


#endif // YAVE_WORLD_NODEREF_H

