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
#ifndef EDITOR_CONTEXT_SCENEDATA_H
#define EDITOR_CONTEXT_SCENEDATA_H

#include <editor/editor.h>

#include <yave/scene/Scene.h>
#include <yave/scene/SceneView.h>

namespace editor {

class SceneData : public ContextLinked, NonCopyable {

	public:
		SceneData(ContextPtr ctx);

		Scene& scene();
		SceneView& view();

		math::Vec3 to_screen_pos(const math::Vec3& world);
		math::Vec2 to_window_pos(const math::Vec3& world);

		bool is_scene_empty() const;

		void save(std::string_view filename);
		void load(std::string_view filename);

		StaticMeshInstance* add(AssetId id);
		StaticMeshInstance* add(std::string_view name);

		void flush();

	private:
		Scene _scene;
		SceneView _scene_view;

		AssetPtr<Material> _default_material;

		core::Vector<std::unique_ptr<StaticMeshInstance>> _to_add;
};

}

#endif // EDITOR_CONTEXT_SCENEDATA_H
