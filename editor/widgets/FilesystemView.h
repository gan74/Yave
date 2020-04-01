/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

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
#ifndef EDITOR_WIDGETS_FILESYSTEMVIEW_H
#define EDITOR_WIDGETS_FILESYSTEMVIEW_H

#include <editor/ui/Widget.h>
#include <yave/utils/FileSystemModel.h>

#include <y/core/Chrono.h>

namespace editor {

class FileSystemView : public Widget {
	public:
		enum class EntryType {
			Directory,
			File
		};

		struct Entry {
			core::String name;
			EntryType type;
			core::String icon;
		};

		FileSystemView(const FileSystemModel* fs = nullptr);

		void refresh() override;
		void set_allow_modify_filesystem(bool allow);

		void set_path(std::string_view path);
		const core::String& path() const;

		void set_filesystem(const FileSystemModel* model);
		const FileSystemModel* filesystem() const;


	protected:
		usize hoverred_index() const;
		const Entry* entry(usize index) const;

		void paint_ui(CmdBufferRecorder&, const FrameToken&) override;

		virtual void update();
		virtual void draw_context_menu();
		virtual core::Result<core::String> entry_icon(const core::String&, EntryType type) const;

		virtual void path_changed() {}
		virtual void entry_hoverred(const Entry*) {}
		virtual void entry_clicked(const Entry& entry);

	private:
		bool process_context_menu();

		const FileSystemModel* _filesystem = nullptr;

		bool _refresh = false;
		bool _allow_modify = true;

		usize _hovered = usize(-1);
		core::Vector<Entry> _entries;

		core::String _current_path;

		static constexpr auto update_duration = core::Duration::seconds(1.0);

		core::Chrono _update_chrono;
};

}

#endif // EDITOR_WIDGETS_FILESYSTEMVIEW_H
