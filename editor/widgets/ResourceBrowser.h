/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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


namespace editor {


class ResourceBrowser : public Widget {

    editor_widget(ResourceBrowser, "View")

    public:
        ResourceBrowser();
        ResourceBrowser(std::string_view title);

        AssetId asset_id(std::string_view name) const;
        AssetType asset_type(AssetId id) const;

        template<typename F>
        void set_filter_delegate(F&& f) {
            _fs_view.set_filter_delegate(y_fwd(f));
        }

        template<typename F>
        void set_selected_delegate(F&& f) {
            _selected_delegate = y_fwd(f);
        }

    protected:
        void on_gui() override;

    private:
        void draw_import_menu();
        void draw_path_bar();
        core::Result<core::String> draw_path_bar_element(std::string_view path);

        FileSystemView _fs_view;

        std::function<bool(AssetId)> _selected_delegate = [](AssetId) { return false; };
};

}

#endif // EDITOR_WIDGETS_RESOURCEBROWSER_H

