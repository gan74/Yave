/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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

#include <editor/Widget.h>
#include <editor/utils/ui.h>

#include <yave/utils/FileSystemModel.h>

#include <y/core/Chrono.h>
#include <y/core/Vector.h>
#include <y/core/Result.h>

namespace editor {

class FileSystemView : public Widget {
    using EntryType = FileSystemModel::EntryType;

    struct Entry {
        core::String full_name;
        core::String name;
        EntryType type;
        UiIcon icon;

        inline std::weak_ordering operator<=>(const Entry& other) const {
            return std::tie(type, name) <=> std::tie(other.type, other.name);
        }
    };

    struct TreeNode {
        core::String full_name;
        core::String name;
        core::Vector<TreeNode> children;

        bool expanded = false;
        bool to_delete = false;

        inline std::weak_ordering operator<=>(const TreeNode& other) const {
            return name.view() <=> other.name.view();
        }
    };

    public:
        FileSystemView(const FileSystemModel* fs = nullptr, std::string_view name = "File browser");

        void refresh() override;

        void set_allow_modify(bool modify);
        void set_split_mode(bool split);

        const FileSystemModel* filesystem() const;

        core::String root_path() const;

        void set_path(const core::String& path);
        const core::String& path() const;

        template<typename F>
        void set_filter_delegate(F&& f) {
            _filter_delegate = y_fwd(f);
            _need_update = true;
        }

        template<typename F>
        void set_icon_delegate(F&& f) {
            _icon_delegate = y_fwd(f);
            _need_update = true;
        }

        template<typename F>
        void set_hoverred_delegate(F&& f) {
            _hoverred_delegate = y_fwd(f);
        }

        template<typename F>
        void set_clicked_delegate(F&& f) {
            _clicked_delegate = y_fwd(f);
        }

        template<typename F>
        void set_on_update(F&& f) {
            _on_update = y_fwd(f);
        }

    protected:
        void on_gui() override;

    private:
        bool process_context_menu();

        void update();
        void set_filesystem(const FileSystemModel* fs);

        void update_nodes(std::string_view path, core::Vector<TreeNode>& nodes);
        void expand_node_path(std::string_view path, core::Vector<TreeNode>& nodes);
        void draw_node(TreeNode &node);

        usize hoverred_index() const;
        const Entry* entry(usize index) const;

        void draw_context_menu();

        void entry_clicked(const Entry& entry);

        static constexpr auto update_duration = core::Duration::seconds(2.0);

        const FileSystemModel* _filesystem = nullptr;
        core::String _current_path;

        std::function<bool(const core::String&, EntryType)> _filter_delegate;
        std::function<UiIcon(const core::String&, EntryType)> _icon_delegate;

        std::function<bool(const core::String&, EntryType)> _clicked_delegate   = [](const core::String&, EntryType) { return false; };
        std::function<void(const core::String&, EntryType)> _hoverred_delegate  = [](const core::String&, EntryType) {};

        std::function<void()> _on_update = [] {};

        usize _hovered = usize(-1);
        core::Vector<Entry> _entries;
        core::Vector<TreeNode> _cached_nodes;

        core::Chrono _update_chrono;

        bool _need_update = true;
        bool _allow_modify = true;
        bool _split_mode = false;
};

}

#endif // EDITOR_WIDGETS_FILESYSTEMVIEW_H

