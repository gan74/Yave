/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include "FileSystemView.h"

#include <yave/assets/AssetId.h>

#include <y/core/FixedArray.h>


namespace editor {

class ResourceBrowser : public FileSystemView {

    editor_widget(ResourceBrowser, "View")

    public:
        ResourceBrowser();

    protected:
        ResourceBrowser(std::string_view title);

        void on_gui() override;

        AssetId asset_id(std::string_view name) const;
        AssetType read_file_type(AssetId id) const;

        virtual void asset_selected(AssetId) {}

    protected:
        void update() override;

        void draw_context_menu() override;
        void path_changed() override;

        core::Result<core::String> entry_icon(const core::String& name, EntryType type) const override;
        void entry_hoverred(const Entry* entry) override;
        void entry_clicked(const Entry& entry) override;

        bool is_searching() const;

    private:
        void draw_search_results();
        void draw_top_bar();
        void draw_import_menu();

        void update_search();

        core::Vector<core::String> _path_pieces;
        core::Vector<core::String> _jump_menu;

        std::unique_ptr<core::Vector<Entry>> _search_results;
        core::FixedArray<char> _search_pattern = core::FixedArray<char>(256);

        core::String _set_path_deferred;
        AssetId _preview_id;
};

}

#endif // EDITOR_WIDGETS_RESOURCEBROWSER_H

