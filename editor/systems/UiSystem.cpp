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

#include "UiSystem.h"
#include "RenderingSystem.h"

#include <editor/components/UiComponent.h>

#include <yave/ecs/EntityWorld.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/utils.h>


namespace editor {

UiSystem::UiSystem() : ecs::System("UiSystem") {
}

void UiSystem::setup(ecs::EntityWorld& world) {
    unused(world);
}

void UiSystem::tick(ecs::EntityWorld& world) {
    y_profile();

    // RenderingSystem* rendering = world.find_system<RenderingSystem>();
    // CmdBufferRecorder recorder = create_disposable_cmd_buffer(rendering->device());

    // Needed to avoid problem when inserting new components
    const core::Vector<ecs::EntityId> ids(world.component_ids<UiComponent>());
    for(const ecs::EntityId id : ids) {
        UiComponent* ui = world.component<UiComponent>(id);
        if(ui->widget) {
            ui->widget->draw_gui_inside();
        }
    }
}

}

