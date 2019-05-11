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
#ifndef EDITOR_CONTEXT_SCENEDATA_H
#define EDITOR_CONTEXT_SCENEDATA_H

#include <editor/editor.h>

#include <yave/scene/Scene.h>
#include <yave/scene/SceneView.h>

namespace editor {

class SceneData : public ContextLinked, NonMovable {

	public:
		SceneData(ContextPtr ctx);

		void set_scene_view(SceneView* scene);
		void remove_scene_view(SceneView* scene);

		Scene& scene();
		SceneView& scene_view();
		SceneView& default_scene_view();

		bool is_scene_empty() const;

		void save(std::string_view filename);
		void load(std::string_view filename);
		void set(Scene&& scene);

		void flush_reload();

	private:
		Scene _scene;
		SceneView _default_scene_view;
		SceneView* _scene_view = nullptr;
};

static_assert(!std::is_move_assignable_v<SceneData>);

}

#endif // EDITOR_CONTEXT_SCENEDATA_H
