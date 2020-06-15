/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include "entities.h"

#include <yave/ecs/EntityWorld.h>

#include <imgui/yave_imgui.h>

namespace editor {

std::string_view entity_icon(const ecs::EntityWorld& world, ecs::EntityId id) {
	if(world.has<StaticMeshComponent>(id)) {
		return ICON_FA_CUBE;
	}

	if(world.has<PointLightComponent>(id)) {
		return ICON_FA_LIGHTBULB;
	}

	if(world.has<SpotLightComponent>(id)) {
		return ICON_FA_VIDEO;
	}

	return ICON_FA_DATABASE;
}

}
