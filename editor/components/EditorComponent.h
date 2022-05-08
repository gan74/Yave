/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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
#ifndef EDITOR_COMPONENTS_EDITORCOMPONENT_H
#define EDITOR_COMPONENTS_EDITORCOMPONENT_H

#include <editor/editor.h>

#include <yave/assets/AssetId.h>
#include <yave/ecs/ecs.h>

#include <y/core/String.h>
#include <y/core/Vector.h>

#include <y/reflect/reflect.h>

namespace editor {

class EditorComponent {
    public:
        EditorComponent() = default;
        EditorComponent(core::String name);

        const core::String& name() const;
        void set_name(core::String name);

        bool has_parent() const;
        bool is_collection() const;
        ecs::EntityId parent() const;
        core::Span<ecs::EntityId> children() const;

        math::Vec3& euler();

        void set_hidden_in_editor(bool hide);
        bool is_hidden_in_editor() const;

        void set_parent_prefab(AssetId id);
        AssetId parent_prefab() const;
        bool is_prefab() const;

        y_reflect(_name, _prefab, _hide_in_editor, _parent, _children, _is_collection)

    private:
        core::String _name = "Unnamed entity";
        AssetId _prefab;

        bool _hide_in_editor = false;
        math::Vec3 _euler;

    private:
        friend class EditorWorld;

        ecs::EntityId _parent;
        core::Vector<ecs::EntityId> _children;
        bool _is_collection = false;
};

}

#endif // EDITOR_COMPONENTS_EDITORCOMPONENT_H

