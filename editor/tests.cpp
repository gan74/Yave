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

#include <editor/editor.h>

#include <yave/ecs/EntityWorld.h>

#include <y/utils/log.h>
#include <y/test/test.h>

namespace editor {

y_test_func("Entity world") {
    ecs::EntityWorld world;

    world.create_entity(ecs::StaticArchetype<int, double, bool>());
    world.create_entity(ecs::StaticArchetype<int, double, bool>());
    world.create_entity(ecs::StaticArchetype<int, double>());
    world.create_entity(ecs::StaticArchetype<double, bool>());
    world.create_entity(ecs::StaticArchetype<int>());

    y_test_assert(world.query<int>().size() == 4);
    y_test_assert(world.query<double>().size() == 4);
    y_test_assert(world.query<bool>().size() == 3);

    y_test_assert(world.query<ecs::Changed<int>>().size() == 4);

    world.tick();

    y_test_assert(world.query<ecs::Changed<int>>().size() == 0);
    y_test_assert((world.query<ecs::Mutate<double>, bool>().size()) == 3);
    y_test_assert(world.query<ecs::Changed<double>>().size() == 3);
    y_test_assert((world.query<ecs::Changed<double>, ecs::Not<int>>().size()) == 1);
}

}

