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

#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/SpotLightComponent.h>


namespace yave {

core::Result<float> entity_radius(ecs::EntityWorld& world, ecs::EntityId id) {
	if(const StaticMeshComponent* mesh = world.component<StaticMeshComponent>(id)) {
		if(mesh->mesh()) {
			return core::Ok(mesh->mesh()->radius());
		}
	}

	if(const PointLightComponent* light = world.component<PointLightComponent>(id)) {
		return core::Ok(light->radius());
	}

	if(const SpotLightComponent* light = world.component<SpotLightComponent>(id)) {
		return core::Ok(light->radius());
	}

	return core::Err();
}

core::Result<math::Vec3> entity_position(ecs::EntityWorld& world, ecs::EntityId id) {
	if(const TransformableComponent* tr = world.component<TransformableComponent>(id)) {
		return core::Ok(tr->transform().position());
	}

	return core::Err();
}

}
