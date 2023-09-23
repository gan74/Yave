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

class ComponentPoolBase;


template<typename T>
class ComponentStorage : NonMovable {
    public:
        ~ComponentStorage() {
            if(!is_empty()) {
                destroy();
            }
        }

        u32 generation() const {
            return _metadata.generation;
        }

        const T* get() const {
            y_debug_assert(!is_empty());
            return &_storage.obj;
        }

        T* get_mut() {
            y_debug_assert(!is_empty());
            _metadata.mutate();
            return &_storage.obj;
        }

        bool is_empty() const {
            return !generation();
        }

        template<typename... Args>
        void init(u32 generation, Args&&... args) {
            y_debug_assert(is_empty());
            new(&_storage.obj) T(y_fwd(args)...);
            _metadata.generation = generation;
            y_debug_assert(!is_empty());
        }

        void destroy() {
            y_debug_assert(!is_empty());

            _metadata.clear();
            _storage.obj.~T();

    #ifdef Y_DEBUG
            std::memset(&_storage.obj, 0xFE, sizeof(_storage.obj));
    #endif
        }

    private:
        union Storage {
            Storage() {
    #ifdef Y_DEBUG
                std::memset(&obj, 0xFE, sizeof(obj));
    #endif
            }

            T obj;
        } _storage;

        struct MetaData {
            u32 generation : 31 = 0;
            u32 mutated    : 1  = false;

            void set_generation(u32 gen) {
                y_debug_assert(generation == 0);
                generation = gen;
                mutated = 0;
            }

            void mutate() {
                y_debug_assert(generation != 0);
                mutated = true;
            }

            void clear() {
                generation = 0;
                mutated = 0;
            }
        } _metadata;

        static_assert(sizeof(MetaData) == sizeof(u32));
};




namespace detail {

void* alloc_page(usize size, usize align);
void dealloc_page(void* ptr, usize size);


static constexpr usize component_page_size = 16 * 1024;
static constexpr usize component_page_alignement = component_page_size;


struct PageHeader {
    const ComponentType type;
    ComponentPoolBase* parent = nullptr;

#ifdef Y_DEBUG
    static constexpr u32 fence_value = 0x12345678;
    u32 fence = fence_value;
#endif
};

template<typename T>
class PagePtr;

inline PageHeader* page_header_from_ptr(void* ptr);

template<typename T>
class Page {
    public:
        static constexpr usize component_count = (component_page_size - sizeof(PageHeader)) / sizeof(ComponentStorage<T>);
        static_assert(component_count >= 1);

        detail::PageHeader header;
        std::array<ComponentStorage<T>, component_count> components;

    private:
        friend PagePtr<T>;

        Page(ComponentPoolBase* parent) : header(component_type<T>(), parent) {
            static_assert(offsetof(Page, header) == 0);
            y_always_assert(page_header_from_ptr(&components[component_count - 1]) == &header, "Page alignment failure");
        }
};

template<typename T>
class PagePtr : NonCopyable {
    public:
        PagePtr(ComponentPool<T>* parent) {
            void* alloc = detail::alloc_page(sizeof(Page<T>), detail::component_page_alignement);
            _ptr = new(alloc) Page<T>(parent);
        }

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
        using component_type = std::remove_const_t<T>;
        using storage_type = ComponentStorage<component_type>;

        ComponentRef() = default;

        inline bool is_null() const {
            y_debug_assert(!_ptr == !_generation);
            return !_ptr;
        }

        inline bool is_stale() const {
            y_debug_assert(_ptr);
            return _ptr->generation() != _generation;
        }

        inline T& operator*() const {
            T* p = get();
            y_debug_assert(p);
            return *p;
        }

        inline T* get() const {
            if(is_null()) {
                Y_TODO(use tombstone object with invalid generation?)
                return nullptr;
            }
            if(is_stale()) {
                return nullptr;
            }

            if constexpr(std::is_const_v<T>) {
                return _ptr->get();
            } else {
                return _ptr->get_mut();
            }
        }

    private:
        friend ComponentPool<component_type>;
        friend class UntypedComponentRef;
        friend class UncheckedComponentRef;

        ComponentRef(storage_type* ptr) : _ptr(ptr) {
        }

        storage_type* _ptr = nullptr;
        u32 _generation = 0;
};

class UntypedComponentRef {
    public:
        UntypedComponentRef() = default;

        template<typename T>
        UntypedComponentRef(ComponentRef<T> ref) : _ptr(ref._ptr), _generation(ref._generation) {
            static_assert(!std::is_const_v<T>);
            y_debug_assert(is<T>());
        }

        inline bool is_null() const {
            y_debug_assert(!_ptr == !_generation);
            return !_ptr;
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
            ref._ptr = reinterpret_cast<typename ComponentRef<T>::storage_type*>(_ptr);
            ref._generation = _generation;
            return ref;
        }

        ComponentPoolBase* pool() const {
            return _ptr ? detail::page_header_from_ptr(_ptr)->parent : nullptr;
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
            ref._ptr = reinterpret_cast<typename ComponentRef<T>::storage_type*>(_ptr);
            ref._generation = ref._ptr->generation();
            return ref;
        }

    private:
        void* _ptr = nullptr;
};

}


#endif // YAVE_WORLD_COMPONENTREF_H

