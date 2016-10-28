/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/

#include "StaticMesh.h"

#include <yave/commands/CmdBufferRecorder.h>
#include <yave/material/Material.h>
#include <yave/RenderPass.h>
#include <yave/Device.h>

namespace yave {

StaticMesh::StaticMesh(const AssetPtr<StaticMeshInstance>& instance, const AssetPtr<Material>& material) :
		_instance(instance),
		_material(material),
		_uniform_buffer(_material->get_device(), 1),
		_descriptor_set(_material->get_device(), {Binding(_uniform_buffer)}) {

}

void StaticMesh::draw(CmdBufferRecorder& recorder, const DescriptorSet& vp) {
	recorder.bind_pipeline(_material->compile(recorder.get_current_pass(), recorder.get_viewport()), _descriptor_set, vp);
	recorder.draw(*_instance);
}

void StaticMesh::update_ubo() {
	_uniform_buffer.map()[0] = transform();
}

}
