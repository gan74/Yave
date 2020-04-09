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

#include "StaticMeshComponent.h"

namespace yave {

StaticMeshComponent::StaticMeshComponent(const AssetPtr<StaticMesh>& mesh, const AssetPtr<Material>& material) :
		_async_mesh(mesh),
		_async_material(material) {
}

void StaticMeshComponent::flush_reload() {
	_async_mesh.flush_reload();
	_async_material.flush_reload();
}


void StaticMeshComponent::render(RenderPassRecorder& recorder, const SceneData& scene_data) const {
	if(!_async_material.flushed() || !_async_mesh.flushed()) {
		return;
	}
	y_debug_assert(_async_material->device());
	y_debug_assert(_async_mesh->device());

	if(_async_material->descriptor_set().is_null()) {
		recorder.bind_material(_async_material->mat_template(), {scene_data.descriptor_set});
	} else {
		recorder.bind_material(_async_material->mat_template(), {scene_data.descriptor_set, _async_material->descriptor_set()});
	}

	render_mesh(recorder, scene_data.instance_index);
}

void StaticMeshComponent::render_mesh(RenderPassRecorder& recorder, u32 instance_index) const {
	if(!_async_mesh.flushed()) {
		return;
	}
	
	recorder.bind_buffers(TriangleSubBuffer(_async_mesh->triangle_buffer()), VertexSubBuffer(_async_mesh->vertex_buffer()));
	auto indirect = _async_mesh->indirect_data();
	indirect.setFirstInstance(instance_index);
	recorder.draw(indirect);
}

const AsyncAssetPtr<StaticMesh>& StaticMeshComponent::mesh() const {
	return _async_mesh;
}

const AsyncAssetPtr<Material>& StaticMeshComponent::material() const {
	return _async_material;
}

AsyncAssetPtr<StaticMesh>& StaticMeshComponent::mesh() {
	return _async_mesh;
}

AsyncAssetPtr<Material>& StaticMeshComponent::material() {
	return _async_material;
}

}
