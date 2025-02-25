/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
#ifndef YAVE_ECS_COMPONENTMATRIX_H
#define YAVE_ECS_COMPONENTMATRIX_H

#include "ecs.h"

#include "SparseComponentSet.h"

#include <y/core/Vector.h>
#include <y/core/HashMap.h>
#include <y/core/String.h>
#include <y/core/FixedArray.h>

#include <y/serde3/result.h>
#include <y/serde3/serde.h>
#include <y/reflect/reflect.h>


namespace yave {
namespace ecs {

class ComponentMatrix {
    struct ComponentIndex {
        u64 mask;
        u32 index;
    };

    struct TagSet {
        SparseIdSet ids;
        core::Vector<EntityGroupProvider*> groups;

        y_reflect(TagSet, ids)
    };

    public:
        void clear();

        bool type_exists(ComponentTypeIndex type) const;

        void add_entity(EntityId id);
        void remove_entity(EntityId id);

        bool contains(EntityId id) const;

        bool has_component(EntityId id, ComponentTypeIndex type) const;



        void add_tag(EntityId id, const core::String& tag);
        void remove_tag(EntityId id, const core::String& tag);
        void clear_tag(const core::String& tag);
        bool has_tag(EntityId id, const core::String& tag) const;

        const SparseIdSet* tag_set(const core::String& tag) const;


        template<typename T>
        void has_component(EntityId id) const {
            has_component(id, type_index<T>());
        }

        template<typename T>
        void add_component(EntityId id) {
            // y_debug_assert(has_all_required_components<T>(id));
            add_component(id, type_index<T>());
        }

        template<typename T>
        void remove_component(EntityId id) {
            // y_debug_assert(!is_component_required<T>(id));
            remove_component(id, type_index<T>());
        }


        template<typename T>
        bool has_all_required_components(EntityId id) const {
            if constexpr(HasRequiredComponents<T>) {
                for(const ComponentTypeIndex r : T::required_component_types) {
                    if(!has_component(id, r)) {
                        return false;
                    }
                }
            }
            return true;
        }


        auto tags() const {
            return _tags.keys();
        }

        serde3::Result save_tags(serde3::WritableArchive& arc) const;
        serde3::Result load_tags(serde3::ReadableArchive& arc);

    private:
        friend class EntityWorld;

        ComponentMatrix(usize type_count);

        void register_group(EntityGroupProvider* group);

        void add_component(EntityId id, ComponentTypeIndex type);
        void remove_component(EntityId id, ComponentTypeIndex type);

        ComponentIndex component_index(EntityId id, ComponentTypeIndex type) const;


        u32 _type_count = 0;
        core::Vector<u64> _bits;
        core::Vector<EntityId> _ids;
        core::FixedArray<core::Vector<EntityGroupProvider*>> _groups;
        std::unique_ptr<bool[]> _required;

        core::FlatHashMap<core::String, TagSet> _tags;
};


}
}

#endif // YAVE_ECS_COMPONENTMATRIX_H

