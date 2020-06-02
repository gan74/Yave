/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

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


#include "EntityPrefab.h"

#include "EntityWorld.h"

namespace y {
namespace ecs {

/*EntityID EntityPrefab::add_to_world(EntityWorld& world) const {
	auto infos = core::vector_with_capacity<ComponentRuntimeInfo>(_components.size());
	for(const auto& cont : _components) {
		y_debug_assert(cont);
		infos.emplace_back(cont->create_runtime_info());
	}

	const ArchetypeRuntimeInfo arc = ArchetypeRuntimeInfo::create(infos);

	const EntityID id = world.create_entity(arc);

	y_fatal("...");

	return id;

}*/

}
}
