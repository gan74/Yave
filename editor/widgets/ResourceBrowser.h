/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include <yave/assets/AssetId.h>

#include "SceneImporter.h"
#include "ImageImporter.h"

namespace editor {

class ResourceBrowser : public Widget, public ContextLinked {

	protected:
		struct FileInfo {
			FileInfo(ContextPtr ctx, std::string_view file, std::string_view full);

			 core::String name;
			 core::String full_name;
			 AssetId id;
			 AssetType type;
		};

	private:
		struct DirNode;

		struct DirNode {
			DirNode(std::string_view dir, std::string_view full, DirNode* par = nullptr);

			core::String name;
			core::String full_path;

			core::Vector<FileInfo> files;
			core::Vector<DirNode> children;

			DirNode* parent = nullptr;
			bool up_to_date = false;
		};

	public:
		ResourceBrowser(ContextPtr ctx);

		void set_path(std::string_view path);

		void refresh() override;
		void finish_search();

	protected:
		ResourceBrowser(ContextPtr ctx, std::string_view title);

		virtual void asset_selected(const FileInfo& file);
		virtual bool display_asset(const FileInfo&file) const;

	private:
		void paint_ui(CmdBufferRecorder& recorder, const FrameToken& token) override;

		void paint_tree_view(float width = 0.0f);
		void paint_asset_list(float width = 0.0f);
		void paint_search_results(float width = 0.0f);
		void paint_preview(float width = 0.0f);
		void paint_context_menu();
		void paint_header();
		void paint_path_node(DirNode* node);
		void paint_search_bar();

	private:
		const FileSystemModel* filesystem() const;

		void set_current(DirNode* current);
		void update_node(DirNode* node);
		void update_search();
		void draw_node(DirNode* node, const core::String& name);
		void make_drop_target(const DirNode* node);

		bool need_refresh() const;

		void reset_hover();

		const FileInfo* hovered_file() const;
		const DirNode* hovered_dir() const;


		static constexpr auto update_duration = core::Duration::seconds(5.0);

		core::Chrono _update_chrono;
		bool _refresh = false;

		DirNode _root;
		NotOwner<DirNode*> _current = nullptr;
		usize _current_hovered_index = usize(-1);

		core::FixedArray<char> _search_pattern = core::FixedArray<char>(256);
		std::unique_ptr<DirNode> _search_node;

		core::Vector<core::Functor<void()>> _deferred;

		AssetId _preview_id;
};

}

#endif // EDITOR_WIDGETS_RESOURCEBROWSER_H
