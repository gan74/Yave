/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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
#ifndef EDITOR_UI_UIELEMENT_H
#define EDITOR_UI_UIELEMENT_H

#include <editor/editor.h>

#include <yave/graphics/swapchain/FrameToken.h>

namespace editor {

class UiElement : NonMovable {
	public:
		UiElement(std::string_view title);

		virtual ~UiElement();

		virtual void refresh();

		virtual void paint(CmdBufferRecorder&, const FrameToken&) = 0;

		bool is_visible() const;
		bool is_child() const;
		bool has_children() const;

		void show();
		void close();

		std::string_view title() const;
		core::Span<std::unique_ptr<UiElement>> children() const;


		template<typename T, typename... Args>
		T* add_child(Args&&... args) {
			_children.emplace_back(std::make_unique<T>(y_fwd(args)...));
			_children.last()->_is_child = true;
			return dynamic_cast<T*>(_children.last().get());
		}

	private:
		friend class Ui;

		void set_id(u64 id);
		void set_title(std::string_view title);

		bool has_visible_children() const;

		u64 _id = 0;
		bool _is_child = false;
		core::Vector<std::unique_ptr<UiElement>> _children;

	protected:
		core::String _title_with_id;
		std::string_view _title;
		bool _visible = true;


};

}

#endif // EDITOR_UI_UIELEMENT_H
