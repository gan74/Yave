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
#ifndef EDITOR_CONTEXT_UI_H
#define EDITOR_CONTEXT_UI_H

#include <editor/widgets/MeshImporter.h>

#include <unordered_map>
#include <typeindex>

namespace editor {

class Ui : NonCopyable, public ContextLinked {

	struct Ids {
		core::Vector<u64> released;
		u64 next = 0;
	};

	public:
		Ui(ContextPtr ctx);
		~Ui();

		core::ArrayView<std::unique_ptr<Widget>> widgets() const;

		// don't spam these two: they are synchronous and modal (and now supported outside of win32 right now)
		bool confirm(const char* message);
		void ok(const char* title, const char* message);

		void paint(CmdBufferRecorder& recorder, const FrameToken& token);

		void refresh_all();

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

		template<typename T, typename... Args>
		T* add(Args&&... args) {
			if constexpr(std::is_constructible_v<T, ContextPtr, Args...>) {
				_widgets.emplace_back(std::make_unique<T>(context(), y_fwd(args)...));
			} else {
				_widgets.emplace_back(std::make_unique<T>(y_fwd(args)...));
			}
			Widget* ptr = _widgets.last().get();
			set_id(ptr);
			return dynamic_cast<T*>(ptr);
		}

	private:
		Ids& ids_for(Widget* widget);
		void set_id(Widget* widget);

		core::Vector<std::unique_ptr<Widget>> _widgets;
		std::unordered_map<std::type_index, Ids> _ids;

};

}

#endif // EDITOR_CONTEXT_UI_H
