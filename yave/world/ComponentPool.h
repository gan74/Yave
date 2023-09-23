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
#ifndef YAVE_WORLD_COMPONENTPOOL_H
#define YAVE_WORLD_COMPONENTPOOL_H

#include "ComponentRef.h"

#include <y/core/Vector.h>

namespace yave {

class ComponentPoolBase : NonMovable {
    public:
        virtual ~ComponentPoolBase();

        ComponentType type() const;

        virtual void remove(UntypedComponentRef ref) = 0;

    protected:
        ComponentPoolBase(ComponentType type);

        const ComponentType _type;
};


template<typename T>
class ComponentPool : public ComponentPoolBase {
    using Page = detail::Page<T>;
    using PagePtr = detail::PagePtr<T>;

    public:
        ComponentPool() : ComponentPoolBase(component_type<T>()) {
        }

        void remove(UntypedComponentRef ref) override {
            remove(ref.to_typed<T>());
        }

        template<typename... Args>
        ComponentRef<T> add(Args&&... args) {
            ComponentRef<T> ref = create_ref();
            ref._ptr->init(ref._generation, y_fwd(args)...);
            return ref;
        }

        void remove(ComponentRef<T> ref) {
            y_debug_assert(!ref.is_null());
            y_debug_assert(std::any_of(_pages.begin(), _pages.end(), [&](const auto& page) { return page._ptr == detail::page_from_ptr<T>(ref._ptr); }));
            if(!ref._ptr->is_empty()) {
                ref._ptr->destroy();
                _free << ref;
            }
        }

    private:
        ComponentRef<T> create_ref() {
            if(_free.is_empty()) {
                create_page();
                return create_ref();
            }

            ComponentRef<T> ref = _free.pop();
            ++ref._generation;
            return ref;
        }

        void create_page() {
            PagePtr& page = _pages.emplace_back();
            page.init();
            for(usize i = 0; i != Page::component_count; ++i) {
                _free << ComponentRef<T>(page.get(i));
            }
        }


        core::Vector<PagePtr> _pages;
        core::Vector<ComponentRef<T>> _free;
};

}


#endif // YAVE_WORLD_COMPONENTPOOL_H

