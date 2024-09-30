/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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

#include "ComponentContainer.h"


namespace yave {
namespace ecs {

ComponentContainerBase::ComponentContainerBase(ComponentTypeIndex type_id) : _type_id(type_id) {
}

ComponentContainerBase::~ComponentContainerBase() {
}

ComponentTypeIndex ComponentContainerBase::type_id() const {
    return _type_id;
}

void ComponentContainerBase::add_required_components(EntityId id) const {
    for(ComponentContainerBase* req : _required) {
        req->add_if_not_exist(id);
    }
}

bool ComponentContainerBase::is_component_required(EntityId id) const {
    for(const ComponentTypeIndex req : _required_by) {
        if(_matrix->has_component(id, req)) {
            return true;
        }
    }
    return false;
}

usize ComponentContainerBase::requirements_chain_depth() const {
    usize depth = 0;
    for(ComponentContainerBase* req : _required) {
        depth = std::max(req->requirements_chain_depth() + 1, depth);
    }
    return depth;
}


}
}

