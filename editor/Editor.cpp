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

#include "Editor.h"

#include <yave/ecs/EntityWorld.h>

#include <editor/systems/RenderingSystem.h>
#include <editor/systems/UiSystem.h>


#include <editor/components/UiComponent.h>

namespace editor {

Editor::Editor(DevicePtr dptr) : _world(std::make_unique<ecs::EntityWorld>()) {
    _world->add_system<RenderingSystem>(dptr);
    _world->add_system<UiSystem>();

    {
        const auto id = _world->create_entity();
        UiComponent* c = _world->add_component<UiComponent>(id);
        c->widget = std::make_unique<Widget2>("floop");
    }
}

Editor::~Editor() {
}

void Editor::tick() {
    _world->tick();
}

}
