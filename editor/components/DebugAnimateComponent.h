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
#ifndef EDITOR_COMPONENTS_DEBUGANIMATECOMPONENT_H
#define EDITOR_COMPONENTS_DEBUGANIMATECOMPONENT_H

#include <editor/editor.h>

#include <y/core/Chrono.h>

namespace editor {

class DebugAnimateComponent {
    public:
        DebugAnimateComponent();

        const math::Vec3& axis() const;
        float speed() const;

        bool rotate() const;
        bool translate() const;

        void inspect(ecs::ComponentInspector* inspector);

    private:
        math::Vec3 _axis;
        float _speed = 1.0f;

        bool _rotate = true;
        bool _translate = false;
};

}

#endif // EDITOR_COMPONENTS_DEBUGANIMATECOMPONENT_H

