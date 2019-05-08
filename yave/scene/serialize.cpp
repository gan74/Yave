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
#include <yave/device/Device.h>

#include <y/core/Chrono.h>
#include <y/io/File.h>

namespace yave {

static constexpr u32 scene_file_version = 3;

void Scene::serialize(io::WriterRef writer) const {
	writer->write_one(fs::magic_number);
	writer->write_one(AssetType::Scene);
	writer->write_one(scene_file_version);

	writer->write_one(u32(_statics.size()));
	writer->write_one(u32(_lights.size()));

	for(const auto& mesh : _statics) {
		writer->write_one(mesh->transform());
		writer->write_one(mesh->mesh().id());
		writer->write_one(mesh->material().id());
	}

	for(const auto& light : _lights) {
		writer->write_one(*light);
	}

	for(const auto& renderable : _renderables) {
		log_msg("Can not serialize renderable of type \"" + type_name(*renderable) + "\"", Log::Warning);
	}

}

core::Result<Scene> Scene::load(io::ReaderRef reader, AssetLoader& loader) {
	y_profile();

	try {
		struct Header {
			u32 magic;
			AssetType type;
			u32 version;

			u32 statics;
			u32 lights;

			bool is_valid() const {
				return magic == fs::magic_number &&
					   type == AssetType::Scene &&
					   version == scene_file_version;
			}
		};

		auto header = reader->read_one<Header>();
		if(!header.is_valid()) {
			return core::Err();
		}

		DevicePtr dptr = loader.device();
		AssetPtr<Material> default_mat = make_asset<Material>(dptr->device_resources()[DeviceResources::BasicMaterialTemplate]);

		Scene scene;
		scene.static_meshes().set_min_capacity(header.statics);
		scene.lights().set_min_capacity(header.lights);

		for(u32 i = 0; i != header.statics; ++i) {
			auto transform = reader->read_one<math::Transform<>>();

			AssetId mesh_id = reader->read_one<AssetId>();
			AssetId mat_id = reader->read_one<AssetId>();

			if(mesh_id == AssetId::invalid_id()) {
				log_msg("Skipping asset with invalid mesh id.", Log::Warning);
				continue;
			}

			auto mesh = loader.load<StaticMesh>(mesh_id);
			if(!mesh) {
				log_msg("Unable to load mesh, skipping.", Log::Warning);
				continue;
			}

			core::Result<AssetPtr<Material>> material = mat_id != AssetId::invalid_id() ? loader.load<Material>(mat_id) : core::Ok(default_mat);
			if(!material) {
				log_msg("Unable to load material, skipping.", Log::Warning);
				continue;
			}

			auto inst = std::make_unique<StaticMeshInstance>(std::move(mesh.unwrap()), std::move(material.unwrap()));
			inst->transform() = transform;
			scene.static_meshes().emplace_back(std::move(inst));
		}

		for(u32 i = 0; i != header.lights; ++i) {
			// load as point, then read on top. Maybe change this ?
			auto light = std::make_unique<Light>(Light::Point);
			reader->read_one(*light);
			scene.lights().emplace_back(std::move(light));
		}

		return core::Ok(std::move(scene));
	} catch(...) {
	}

	return core::Err();
}

}
