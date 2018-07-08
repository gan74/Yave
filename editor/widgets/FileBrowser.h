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
#ifndef EDITOR_VIEWS_FILEBROWSER_H
#define EDITOR_VIEWS_FILEBROWSER_H

#include <editor/ui/Frame.h>
#include <editor/ui/Widget.h>

#include <yave/utils/FileSystemModel.h>

#include <y/core/AssocVector.h>
#include <y/core/Functor.h>

namespace editor {

class FileBrowser : public Widget {
	public:
		FileBrowser();

		template<typename F>
		void set_callback(F&& func) {
			_callback = std::forward<F>(func);
		}

		void set_filesystem(NotOwner<FileSystemModelBase*> model);
		void set_path(std::string_view path);

	private:
		static constexpr usize buffer_capacity = 1024;

		void done();
		void cancel();

		core::String full_path() const;
		std::string_view path() const;

		void paint_ui(CmdBufferRecorder<>&, const FrameToken&) override;

		FileSystemModelBase* _model = nullptr;

		core::Vector<core::String> _entries;

		std::array<char, buffer_capacity> _path_buffer;
		std::array<char, buffer_capacity> _name_buffer;

		usize _selection = -1;

		core::Function<void(core::String)> _callback;

};

}

#endif // EDITOR_VIEWS_FILEBROWSER_H
