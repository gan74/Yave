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
#ifndef EDITOR_CONTEXT_UI_H
#define EDITOR_CONTEXT_UI_H

#include <editor/editor.h>
#include <editor/widgets/CameraDebug.h>
#include <editor/widgets/SceneDebug.h>
#include <editor/widgets/SettingsPanel.h>
#include <editor/widgets/memoryInfo.h>
#include <editor/widgets/PerformanceMetrics.h>
#include <editor/widgets/FileBrowser.h>
#include <editor/widgets/MeshImporter.h>

namespace editor {

class Ui : public ContextLinked, NonCopyable {
	public:
		Ui(ContextPtr ctx);

		core::ArrayView<std::unique_ptr<Widget>> widgets() const;

		bool confirm(const char* message);
		void ok(const char* title, const char* message);

		void paint(CmdBufferRecorder& recorder, const FrameToken& token);


		template<typename T>
		T* show() {
			for(auto&& e : _widgets) {
				if(T* w = dynamic_cast<T*>(e.get())) {
					w->show();
					return w;
				}
			}
			return add<T>();
		}

		template<typename T>
		T* add() {
			if constexpr(std::is_constructible_v<T, ContextPtr>) {
				_widgets.emplace_back(std::make_unique<T>(context()));
			} else {
				_widgets.emplace_back(std::make_unique<T>());
			}
			return dynamic_cast<T*>(_widgets.last().get());
		}

	private:
		core::Vector<std::unique_ptr<Widget>> _widgets;
};

}

#endif // EDITOR_CONTEXT_UI_H
