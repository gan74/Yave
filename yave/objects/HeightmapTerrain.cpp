/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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

#include "HeightmapTerrain.h"

#include <y/io/File.h>

namespace yave {

static StaticMeshInstance create_mesh(MeshInstancePool& mesh_pool) {
	MeshData data;

	math::Vec3 up{0.0f, 0.0f, 1.0f};
	data.vertices = {{{-1.0f, -1.0f, 0.0f}, up, {0.0f, 0.0f}},
					 {{-1.0f,  1.0f, 0.0f}, up, {0.0f, 1.0f}},
					 {{ 1.0f,  1.0f, 0.0f}, up, {1.0f, 1.0f}},
					 {{ 1.0f, -1.0f, 0.0f}, up, {1.0f, 0.0f}}};
	data.triangles = {{2, 1, 0}, {2, 0, 3}};
	data.radius = data.vertices.first().position.length();

	return mesh_pool.create_static_mesh(data);
}

static Material create_material(DevicePtr dptr, const AssetPtr<Texture>& heightmap) {
	return Material(dptr, MaterialData()
			.set_frag_data(SpirVData::from_file(io::File::open("textured.frag.spv").expected("Unable to load spirv file")))
			.set_vert_data(SpirVData::from_file(io::File::open("terrain.vert.spv").expected("Unable to load spirv file")))
			.set_bindings({Binding(TextureView(*heightmap))})
		);
}

HeightmapTerrain::HeightmapTerrain(MeshInstancePool& mesh_pool, const AssetPtr<Texture>& heightmap) :
		_mesh(create_mesh(mesh_pool)),
		_material(create_material(mesh_pool.device(), heightmap)),
		_heightmap(heightmap) {
}

void HeightmapTerrain::render(const FrameToken&, CmdBufferRecorderBase& recorder, DescriptorList descriptor_sets) const {
	recorder.bind_material(_material, descriptor_sets);
	recorder.draw(_mesh);
}

}
