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
#ifndef YAVE_WORLD_COMPONENTREF_H
#define YAVE_WORLD_COMPONENTREF_H

#include "ComponentType.h"

#include <array>
#include <bit>

namespace yave {

template<typename T>
class ComponentPool;


template<typename T>
struct ComponentStorage : NonMovable {
    ~ComponentStorage() {
        if(!is_empty()) {
            destroy();
        }
    }

    T* get() {
        y_debug_assert(!is_empty());
        return &_storage.obj;
    }

    bool is_empty() const {
        return !_generation;
    }

    template<typename... Args>
    void init(u32 generation, Args&&... args) {
        y_debug_assert(is_empty());
        new(&_storage.obj) T(y_fwd(args)...);
        _generation = generation;
        y_debug_assert(!is_empty());
    }

    void destroy() {
        y_debug_assert(!is_empty());

        _generation = 0;
        _storage.obj.~T();

#ifdef Y_DEBUG
        std::memset(&_storage.obj, 0xFE, sizeof(_storage.obj));
#endif
    }

    union Storage {
        Storage() {
#ifdef Y_DEBUG
            std::memset(&obj, 0xFE, sizeof(obj));
#endif
        }

        T obj;
    } _storage;

    u32 _generation = 0;
};




namespace detail {

void* alloc_page(usize size, usize align);
void dealloc_page(void* ptr, usize size);


static constexpr usize component_page_size = 16 * 1024;
static constexpr usize component_page_alignement = component_page_size;


struct PageHeader {
    const ComponentType type;

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
        static constexpr usize component_count = (component_page_size - sizeof(PageHeader)) / sizeof(ComponentStorage<T>);
        static_assert(component_count >= 1);

        detail::PageHeader header;
        std::array<ComponentStorage<T>, component_count> components;

    private:
        friend PagePtr<T>;

        Page() : header(component_type<T>()) {
            static_assert(offsetof(Page, header) == 0);
        }
};

template<typename T>
class PagePtr : NonCopyable {
    public:
        static_assert(sizeof(Page<T>) <= detail::component_page_size);

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

        bool is_null() const {
            return !_ptr;
        }

        void init() {
            y_debug_assert(!_ptr);
            void* alloc = detail::alloc_page(sizeof(Page<T>), detail::component_page_alignement);
            _ptr = new(alloc) Page<T>();
        }

        ComponentStorage<T>* get(usize index) {
            y_debug_assert(_ptr);
            return &_ptr->components[index];
        }

    private:
        friend ComponentPool<T>;

        Page<T>* _ptr = nullptr;
};


inline PageHeader* page_header_from_ptr(void* ptr) {
    static_assert(sizeof(usize) == sizeof(ptr));
    const usize uptr = std::bit_cast<usize>(ptr);
    const usize start = uptr - (uptr % component_page_alignement);
    PageHeader* header = std::bit_cast<PageHeader*>(start);
    y_debug_assert(header->fence == PageHeader::fence_value);
    return header;
}

template<typename T>
inline Page<T>* page_from_ptr(void* ptr) {
    PageHeader* header = page_header_from_ptr(ptr);
    y_debug_assert(header->type == component_type<T>());
    return reinterpret_cast<Page<T>*>(header);
}

}








template<typename T>
struct ComponentRef {
    public:
        ComponentRef() = default;

        inline bool is_null() const {
            y_debug_assert(!_ptr == !_generation);
            return !_ptr;
        }

        inline const T& operator*() const {
            const T* p = get();
            y_debug_assert(p);
            return *p;
        }

        inline const T* get() const {
            if(is_null()) {
                Y_TODO(use null object with invalid generation?)
                return nullptr;
            }
            return _ptr->_generation == _generation ? _ptr->get() : nullptr;
        }

    private:
        friend ComponentPool<T>;
        friend class UntypedComponentRef;
        friend class UncheckedComponentRef;

        ComponentRef(ComponentStorage<T>* ptr) : _ptr(ptr) {
        }

        ComponentStorage<T>* _ptr = nullptr;
        u32 _generation = 0;
};

class UntypedComponentRef {
    public:
        UntypedComponentRef() = default;

        template<typename T>
        UntypedComponentRef(ComponentRef<T> ref) : _ptr(ref._ptr), _generation(ref._generation) {
            y_debug_assert(is<T>());
        }

        inline ComponentType type() const {
            y_debug_assert(_ptr);
            return detail::page_header_from_ptr(_ptr)->type;
        }

        template<typename T>
        inline bool is() const {
            return _ptr ? detail::page_header_from_ptr(_ptr)->type == component_type<T>() : false;
        }

        template<typename T>
        inline ComponentRef<T> to_typed() const {
            if(!is<T>()) {
                return {};
            }

            return to_typed_unchecked<T>();
        }

        template<typename T>
        inline ComponentRef<T> to_typed_unchecked() const {
            y_debug_assert(is<T>());

            ComponentRef<T> ref;
            ref._ptr = reinterpret_cast<ComponentStorage<T>*>(_ptr);
            ref._generation = _generation;
            return ref;
        }

    private:
        friend class UncheckedComponentRef;

        void* _ptr = nullptr;
        u32 _generation = 0;
};

class UncheckedComponentRef {
    public:
        UncheckedComponentRef() = default;

        UncheckedComponentRef(UntypedComponentRef ref) : _ptr(ref._ptr) {
        }

        inline ComponentType type() const {
            y_debug_assert(_ptr);
            return detail::page_header_from_ptr(_ptr)->type;
        }

        template<typename T>
        inline bool is() const {
            return _ptr ? detail::page_header_from_ptr(_ptr)->type == component_type<T>() : false;
        }

        template<typename T>
        inline ComponentRef<T> to_typed_unchecked() const {
            y_debug_assert(is<T>());

            ComponentRef<T> ref;
            ref._ptr = reinterpret_cast<ComponentStorage<T>*>(_ptr);
            ref._generation = ref._ptr->_generation;
            return ref;
        }

    private:
        void* _ptr = nullptr;
};

}


#endif // YAVE_WORLD_COMPONENTREF_H

