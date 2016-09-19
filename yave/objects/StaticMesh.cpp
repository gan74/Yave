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

/*RecordedCmdBuffer create_cmd_buffer(const RenderPass& render_pass, const StaticMeshInstance& instance, const Material& material) {
	auto recorder =
}*/

StaticMesh::StaticMesh(const AssetPtr<StaticMeshInstance>& instance, const AssetPtr<Material>& material) :
		_instance(instance),
		_material(material) {
}

/*const RecordedCmdBuffer& StaticMesh::get_cmd_buffer(const RenderPass& render_pass) const {
	auto key = recorder.get_current_pass().get_vk_render_pass();
	auto it = core::range(_cmd_buffers).find(key);
	if(it == _cmd_buffers.end()) {
		_cmd_buffers.insert(key, create_cmd_buffer());
		return _cmd_buffers.last().second;
	}
	return it->second;
}*/

void StaticMesh::draw(CmdBufferRecorder& recorder) {
	recorder.bind_pipeline(_material->compile(recorder.get_current_pass(), recorder.get_viewport()));
	recorder.draw(*_instance);
}

}
