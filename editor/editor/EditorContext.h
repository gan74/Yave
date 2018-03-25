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
#ifndef EDITOR_EDITORCONTEXT_H
#define EDITOR_EDITORCONTEXT_H

#include <editor/settings/settings.h>

#include <yave/device/DeviceLinked.h>
#include <yave/objects/Transformable.h>
#include <yave/scene/SceneView.h>

namespace editor {

class EditorContext : public DeviceLinked, NonCopyable {
	public:
		EditorContext(DevicePtr dptr);
		~EditorContext();

		Scene* scene() const;
		SceneView* scene_view() const;

		void save_settings();
		void load_settings();


		CameraSettings camera_settings;

		NotOwner<Transformable*> selected = nullptr;

	private:
		core::Unique<Scene> _scene;
		core::Unique<SceneView> _scene_view;


};

}

#endif // EDITOR_EDITORCONTEXT_H
