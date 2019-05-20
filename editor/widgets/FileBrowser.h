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
#ifndef EDITOR_WIDGETS_FILEBROWSER_H
#define EDITOR_WIDGETS_FILEBROWSER_H

#include <editor/ui/Widget.h>
#include <yave/utils/FileSystemModel.h>

namespace editor {

class FileBrowser final : public Widget {
	enum class EntryType {
		Directory,
		Supported,
		Unsupported
	};

	public:
		FileBrowser(const FileSystemModel* filesystem = nullptr);

		template<typename F>
		void set_selected_callback(F&& func) {
			_callbacks.selected = std::move(func);
		}

		template<typename F>
		void set_canceled_callback(F&& func) {
			_callbacks.canceled = std::move(func);
		}

		void set_filesystem(const FileSystemModel* model);
		void set_path(std::string_view path);
		void set_extension_filter(std::string_view exts);

	private:
		void paint_ui(CmdBufferRecorder&, const FrameToken&) override;

		void done(const core::String& filename);
		void cancel();

		core::String full_path() const;
		std::string_view path() const;

		const FileSystemModel* _filesystem = nullptr;

		core::Vector<core::String> _extensions;
		core::Vector<std::pair<core::String, EntryType>> _entries;

		static constexpr usize buffer_capacity = 1024;
		std::array<char, buffer_capacity> _path_buffer;
		std::array<char, buffer_capacity> _name_buffer;

		core::String _last_path;

		struct {
			core::Function<bool(const core::String&)> selected = [](const auto&) { return false; };
			core::Function<bool()> canceled = [] { return true; };
		} _callbacks;
};

}

#endif // EDITOR_WIDGETS_FILEBROWSER_H
