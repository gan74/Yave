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
#ifndef YAVE_ECS_WORLDCOMPONENTCONTAINER_H
#define YAVE_ECS_WORLDCOMPONENTCONTAINER_H

#include "ecs.h"

#include <y/serde3/traits.h>
#include <y/serde3/poly.h>

namespace yave {
namespace ecs {

class WorldComponentContainerBase : NonMovable {
    public:
        virtual ~WorldComponentContainerBase() {
        }

        template<typename T>
        T* try_get();

        template<typename T>
        const T* try_get() const;

        y_serde3_poly_abstract_base(WorldComponentContainerBase)
};

template<typename T>
class WorldComponentContainer final : public WorldComponentContainerBase {
    public:
        WorldComponentContainer() : WorldComponentContainerBase() {
        }

        T& get() {
            return _component;
        }

        const T& get() const {
            return _component;
        }

        y_no_serde3_expr(serde3::has_no_serde3_v<T>)

        y_reflect(WorldComponentContainer, _component)
        y_serde3_poly(WorldComponentContainer)

    private:
        T _component;
};


template<typename T>
T* WorldComponentContainerBase::try_get() {
    if(auto* typed = dynamic_cast<WorldComponentContainer<T>*>(this)) {
        return &typed->get();
    }
    return nullptr;
}

template<typename T>
const T* WorldComponentContainerBase::try_get() const {
    if(const auto* typed = dynamic_cast<const WorldComponentContainer<T>*>(this)) {
        return &typed->get();
    }
    return nullptr;
}

}
}

#endif // YAVE_ECS_COMPONENTCONTAINER_H

