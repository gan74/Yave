/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include "serialize.h"

#include <yave/scene/Scene.h>
#include <yave/material/Material.h>

#include <unordered_map>

namespace yave {


template<typename T>
static std::unordered_map<const T*, u32> serialize(io::WriterRef writer, const AssetLoader<T>& loader) {
	std::unordered_map<const T*, u32> ids;

	writer->write_one(u32(loader.size()));
	for(const auto& asset : loader) {
		u32 id = u32(ids.size());
		ids[asset.second.get()] = id;

		serialize(writer, asset.first);
	}

	return ids;
}

static void deserialize(io::ReaderRef reader, core::Vector<core::String>& names) {
	u32 size = reader->read_one<u32>().unwrap();
	names.set_min_capacity(size);

	for(u32 i = 0; i != size; ++i) {
		names.emplace_back();
		deserialize(reader, names.last());
	}
}

// --------------------------------------------------------  serialize  --------------------------------------------------------

void serialize(io::WriterRef writer, const core::String& str) {
	writer->write_one(u32(str.size()));
	writer->write(str.data(), str.size());
}


void serialize(io::WriterRef writer, const Scene& scene, const AssetLoader<StaticMesh>& mesh_loader)  {
	auto mesh_ids = serialize(writer, mesh_loader);

	writer->write_one(u32(scene.static_meshes().size()));
	for(const auto& mesh : scene.static_meshes()) {
		auto mesh_id = mesh_ids[mesh->mesh().get()];
		writer->write_one(mesh_id);
		writer->write_one(mesh->transform());
	}

	for(const auto& renderable : scene.renderables()) {
		log_msg("Can not serialize renderable of type \"" + type_name(*renderable) + "\"", Log::Warning);
	}

	writer->write_one(u32(scene.lights().size()));
	for(const auto& light : scene.lights()) {
		writer->write_one(*light);
	}
}


// -------------------------------------------------------- deserialize --------------------------------------------------------

void deserialize(io::ReaderRef reader, core::String& str) {
	u32 size = reader->read_one<u32>().unwrap();
	str = core::String(nullptr, size);
	reader->read(str.data(), size).unwrap();
	str[str.size()] = 0;
}

void deserialize(io::ReaderRef reader, Scene& scene, AssetLoader<StaticMesh>& mesh_loader) {
	core::Vector<core::String> mesh_names;
	deserialize(reader, mesh_names);

	DevicePtr dptr = mesh_loader.device();

	auto material = make_asset<Material>(dptr, MaterialData()
			.set_frag_data(SpirVData::from_file(io::File::open("basic.frag.spv").expected("Unable to load spirv file.")))
			.set_vert_data(SpirVData::from_file(io::File::open("basic.vert.spv").expected("Unable to load spirv file.")))
		);

	u32 mesh_count = reader->read_one<u32>().unwrap();
	for(u32 i = 0; i != mesh_count; ++i) {
		u32 mesh_id = reader->read_one<u32>().unwrap();
		auto mesh = mesh_loader.from_file(mesh_names[mesh_id]).unwrap();

		auto inst = std::make_unique<StaticMeshInstance>(mesh, material);
		reader->read_one(inst->transform()).unwrap();
		scene.static_meshes().emplace_back(std::move(inst));
	}

	u32 light_count = reader->read_one<u32>().unwrap();
	for(u32 i = 0; i != light_count; ++i) {
		// load as point, then read on top. Maybe change this ?
		auto light = std::make_unique<Light>(Light::Point);
		reader->read_one(*light).unwrap();
		scene.lights().emplace_back(std::move(light));
	}
}

}
