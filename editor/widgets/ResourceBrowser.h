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
#ifndef EDITOR_WIDGETS_RESOURCEBROWSER_H
#define EDITOR_WIDGETS_RESOURCEBROWSER_H

#include "MeshImporter.h"
#include "ImageImporter.h"

namespace editor {

class ResourceBrowser : public Widget, public ContextLinked {

	protected:
		struct FileInfo {
			FileInfo(ContextPtr ctx, std::string_view file, std::string_view full);

			const core::String name;
			const core::String full_name;
			const AssetId id;
			const AssetType type;
		};

	private:
		struct DirNode {
			DirNode(std::string_view dir, std::string_view full, DirNode* par = nullptr);

			const core::String name;
			const core::String full_path;

			core::Vector<FileInfo> files;
			core::Vector<DirNode> children;

			DirNode* parent;
			bool up_to_date = false;
		};

	public:
		ResourceBrowser(ContextPtr ctx);

		void refresh() override;

	protected:
		ResourceBrowser(ContextPtr ctx, std::string_view title);

		virtual void asset_selected(const FileInfo& file);
		virtual bool display_asset(const FileInfo&file) const;

		static std::string_view icon_for_type(AssetType type);

	private:
		void paint_ui(CmdBufferRecorder& recorder, const FrameToken& token) override;

		void paint_tree_view(float width = 0.0f);
		void paint_asset_list(float width = 0.0f);
		void paint_preview(float width = 0.0f);
		void paint_context_menu();

	private:
		const FileSystemModel* filesystem() const;

		void set_current(DirNode* current);
		void update_node(DirNode* node);
		void draw_node(DirNode* node, const core::String& name);

		bool need_refresh() const;

		void reset_hover();

		const FileInfo* hovered_file() const;
		const DirNode* hovered_dir() const;


		static constexpr auto update_duration = core::Duration::seconds(5.0f);

		core::Chrono _update_chrono;
		bool _refresh = false;

		DirNode _root;
		NotOwner<DirNode*> _current = nullptr;
		usize _current_hovered_index = usize(-1);
};

}

#endif // EDITOR_WIDGETS_RESOURCEBROWSER_H
