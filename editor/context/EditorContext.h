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
#ifndef EDITOR_CONTEXT_EDITORCONTEXT_H
#define EDITOR_CONTEXT_EDITORCONTEXT_H

#include <editor/settings/settings.h>

#include <yave/assets/AssetLoader.h>

#include <yave/device/DeviceLinked.h>
#include <yave/objects/Transformable.h>
#include <yave/scene/SceneView.h>

namespace editor {

class EditorContext : public DeviceLinked, NonCopyable {

	public:
		struct Icons {
			TextureView light;
			TextureView save;
		};

		EditorContext(DevicePtr dptr);
		~EditorContext();

		Scene* scene() const;
		SceneView* scene_view() const;

		void save_scene();
		void load_scene();
		void clear_scene();

		void save_settings();
		void load_settings();

		void set_selected(Light* sel);
		void set_selected(Transformable* sel);
		void set_selected(std::nullptr_t);

		Transformable* selected() const;
		Light* selected_light() const;

		math::Vec2 to_screen_pos(const math::Vec3& world);
		math::Vec2 to_window_pos(const math::Vec3& world);

		Icons* icons() const;

		CameraSettings camera_settings;

		AssetLoader<Texture> texture_loader;
		AssetLoader<StaticMesh> mesh_loader;

	private:
		std::unique_ptr<Scene> _scene;
		std::unique_ptr<SceneView> _scene_view;

		NotOwner<Transformable*> _selected = nullptr;
		NotOwner<Light*> _selected_light = nullptr;

		core::Vector<Texture> _icon_textures;
		mutable Icons _icons;


};

}

#endif // EDITOR_CONTEXT_EDITORCONTEXT_H
