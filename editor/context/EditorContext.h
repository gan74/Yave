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

#include <editor/editor.h>

#include <y/core/Functor.h>
#include <yave/device/DeviceLinked.h>
#include <yave/utils/FileSystemModel.h>

#include "Settings.h"
#include "SceneData.h"
#include "Icons.h"
#include "Loader.h"
#include "Selection.h"
#include "Ui.h"

#include <mutex>

namespace editor {

class EditorContext : public DeviceLinked, NonCopyable {

	public:
		EditorContext(DevicePtr dptr);
		~EditorContext();


		void defer(core::Function<void()>&& func);
		void flush_deferred();


		const FileSystemModel* filesystem() const {
			return _filesystem.get() ? _filesystem.get() : FileSystemModel::local_filesystem();
		}

		Settings& settings() {
			return _setting;
		}

		SceneData& scene() {
			return _scene;
		}

		Selection& selection() {
			return _selection;
		}

		Loader& loader() {
			return _loader;
		}

		Icons& icons() {
			return _icons;
		}

		Ui& ui() {
			return _ui;
		}

	private:
		std::unique_ptr<FileSystemModel> _filesystem;

		std::mutex _deferred_lock;
		core::Vector<core::Function<void()>> _deferred;
		bool _is_flushing_deferred = false;

		Settings _setting;
		Loader _loader;
		SceneData _scene;
		Selection _selection;
		Icons _icons;
		Ui _ui;

};

}

#endif // EDITOR_CONTEXT_EDITORCONTEXT_H
