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

#include "scenes.h"

#include <y/io/File.h>

#include <yave/objects/Light.h>
#include <yave/objects/SkinnedMeshInstance.h>

#include <yave/animations/Animation.h>

namespace editor {

void add_skinned_mesh(Scene* scene, AssetLoader<Texture>& tex_loader, AssetLoader<StaticMesh>&) {
	DevicePtr dptr = tex_loader.device();

	auto material = dptr->default_resources()[DefaultResources::BasicSkinnedMaterial];

	try {
		auto mesh_data = MeshData::deserialized(io::File::open("../meshes/beta.fbx.ym").expected("Unable to open mesh file."));
		auto mesh = make_asset<SkinnedMesh>(dptr, mesh_data);

		log_msg(fmt("% triangles loaded", mesh_data.triangles().size()));

		{
			auto instance = std::make_unique<SkinnedMeshInstance>(mesh, material);
			instance->position() = {0.0f, 100.0f, -instance->radius() * 0.5f};
			try {
				auto animation = make_asset<Animation>(Animation::deserialized(io::File::open("../meshes/walk.fbx.ya").expected("Unable to open animation file.")));
				instance->animate(animation);
			} catch(std::exception& e) {
				log_msg(e.what(), Log::Error);
			}
			scene->renderables() << std::move(instance);
		}
	} catch(std::exception& e) {
		log_msg(fmt("Exception while loading: %", e.what()), Log::Error);
	}
}

void fill_scene(Scene* scene, AssetLoader<Texture>& tex_loader, AssetLoader<StaticMesh>& mesh_loader) {

	DevicePtr dptr = tex_loader.device();

	{
		Light l(Light::Directional);
		l.transform().set_basis(math::Vec3{1.0f, 1.0f, 3.0f}.normalized(), {1.0f, 0.0f, 0.0f});
		l.color() = math::Vec3{1.0f};
		scene->lights() << std::make_unique<Light>(std::move(l));
	}

	{
		Light l(Light::Point);
		l.color() = math::Vec3{1.0f, 0.0f, 0.0f};
		l.intensity() = 10000.0f;
		l.position().z() = 100.0f;
		l.radius() = 100.0f;
		scene->lights() << std::make_unique<Light>(std::move(l));
	}

	add_skinned_mesh(scene, tex_loader, mesh_loader);

	{
		auto material = dptr->default_resources()[DefaultResources::BasicMaterial];
		//auto mesh_data = MeshData::from_file(io::File::open("../meshes/cube.obj.ym").expected("Unable to open mesh file."));
		//auto mesh = AssetPtr<StaticMesh>(StaticMesh(dptr, mesh_data));

		try {
			AssetPtr<StaticMesh> mesh = mesh_loader.import("cube.ym", "../meshes/cube.obj.ym");

			{
				auto instance = std::make_unique<StaticMeshInstance>(mesh, material);
				instance->transform() = math::Transform<>(math::Vec3(0.0f, 0.0f, 0.0f), math::identity(), math::Vec3(0.1f));
				scene->static_meshes() << std::move(instance);
			}

			{
				auto instance = std::make_unique<StaticMeshInstance>(mesh, material);
				instance->transform() = math::Transform<>(math::Vec3(100.0f, 0.0f, 0.0f), math::identity(), math::Vec3(0.1f));
				scene->static_meshes() << std::move(instance);
			}

			{
				auto instance = std::make_unique<StaticMeshInstance>(mesh, material);
				instance->transform() = math::Transform<>(math::Vec3(0.0f, 100.0f, 0.0f), math::identity(), math::Vec3(0.1f));
				scene->static_meshes() << std::move(instance);
			}

			{
				auto instance = std::make_unique<StaticMeshInstance>(mesh, material);
				instance->transform() = math::Transform<>(math::Vec3(0.0f, 0.0f, 100.0f), math::identity(), math::Vec3(0.1f));
				scene->static_meshes() << std::move(instance);
			}
		} catch(std::exception& e) {
			log_msg(fmt("Exception while loading: %", e.what()), Log::Error);
		}

	}

	/*std::unique_ptr scene_view = new SceneView(*scene);
	{
		float dist = 200.0f;
		auto& camera = scene_view->camera();

		auto cam_pos = math::Vec4(dist, 0, 0, 1);
		auto cam_up = math::Vec4(0, 0, 1, 0);

		camera.set_view(math::look_at(cam_pos.to<3>() / cam_pos.w(), math::Vec3(), cam_up.to<3>()));
		camera.set_proj(math::perspective(math::to_rad(60.0f), 4.0f / 3.0f, 1.0f));
	}
	return std::make_pair(std::move(scene), std::move(scene_view));*/
}

}
