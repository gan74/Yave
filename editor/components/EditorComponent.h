/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include <y/core/String.h>
#include <y/reflect/reflect.h>

namespace editor {

class EditorComponent {
    public:
        EditorComponent() = default;
        EditorComponent(core::String name);

        const core::String& name() const;
        void set_name(core::String name);

        math::Vec3& euler();

        void set_hidden_in_editor(bool hide);
        bool is_hidden_in_editor() const;

        y_reflect(_name, _hide_in_editor)

    private:
        core::String _name = "Unnamed entity";
        math::Vec3 _euler;

        bool _hide_in_editor = false;
};

}

#endif // EDITOR_COMPONENTS_EDITORCOMPONENT_H

