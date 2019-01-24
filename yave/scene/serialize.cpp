/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include "Scene.h"

#include <yave/material/Material.h>

#include <y/core/Chrono.h>
#include <y/io/File.h>

namespace yave {

void Scene::serialize(io::WriterRef writer) const {
	writer->write_one(fs::magic_number);
	writer->write_one(AssetType::Scene);
	u32 version = 2;
	writer->write_one(version);

	writer->write_one(u32(_statics.size()));
	writer->write_one(u32(_lights.size()));

	for(const auto& mesh : _statics) {
		AssetId id = mesh->mesh().id();
		if(id == AssetId::invalid_id()) {
			log_msg("Asset with invalid id, skipping.", Log::Warning);
			continue;
		}
		writer->write_one(id);
		writer->write_one(mesh->transform());
	}

	for(const auto& light : _lights) {
		writer->write_one(*light);
	}

	for(const auto& renderable : _renderables) {
		log_msg("Can not serialize renderable of type \"" + type_name(*renderable) + "\"", Log::Warning);
	}

}

Scene Scene::deserialized(io::ReaderRef reader, AssetLoader<StaticMesh>& mesh_loader, const AssetPtr<Material>& default_material) {
	y_profile();

	struct Header {
		u32 magic;
		AssetType type;
		u32 version;

		u32 statics;
		u32 lights;

		bool is_valid() const {
			return magic == fs::magic_number &&
				   type == AssetType::Scene &&
				   version == 2;
		}
	};

	auto header = reader->read_one<Header>();
	if(!header.is_valid()) {
		y_throw("Invalid header.");
	}

	Scene scene;
	scene.static_meshes().set_min_capacity(header.statics);
	scene.lights().set_min_capacity(header.lights);

	{
		for(u32 i = 0; i != header.statics; ++i) {
			auto id = reader->read_one<AssetId>();
			auto transform = reader->read_one<math::Transform<>>();

			if(id == AssetId::invalid_id()) {
				log_msg("Skipping asset with invalid id.", Log::Warning);
				continue;
			}

			try {
				auto mesh = mesh_loader.load(id);
				auto inst = std::make_unique<StaticMeshInstance>(mesh, default_material);
				inst->transform() = transform;
				scene.static_meshes().emplace_back(std::move(inst));
			} catch(std::exception& e) {
				log_msg(fmt("Unable to load asset: %, Skipping.", e.what()), Log::Warning);
				continue;
			}
		}
	}

	{
		for(u32 i = 0; i != header.lights; ++i) {
			// load as point, then read on top. Maybe change this ?
			auto light = std::make_unique<Light>(Light::Point);
			reader->read_one(*light);
			scene.lights().emplace_back(std::move(light));
		}
	}

	return scene;
}

}
