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
#ifndef EDITOR_WIDGETS_FILESYSTEMVIEW_H
#define EDITOR_WIDGETS_FILESYSTEMVIEW_H

#include <editor/Widget.h>

#include <y/core/Chrono.h>
#include <y/core/Vector.h>
#include <y/core/Result.h>

namespace editor {

class FileSystemView : public Widget {
    public:
        void refresh() override;

        const FileSystemModel* filesystem() const;

        void set_path(const core::String& path);
        const core::String& path() const;

    protected:
        enum class EntryType {
            Directory,
            File
        };

        struct Entry {
            core::String name;
            EntryType type;
            core::String icon;

            bool operator<(const Entry& other) const {
                return std::tie(type, name) < std::tie(other.type, other.name);
            }
        };

        FileSystemView(const FileSystemModel* fs = nullptr, std::string_view name = "File browser");
        void set_filesystem(const FileSystemModel* model);

        usize hoverred_index() const;
        const Entry* entry(usize index) const;
        core::String entry_full_name(const Entry& entry) const;


        virtual void update();
        virtual void draw_context_menu();
        virtual core::Result<core::String> entry_icon(const core::String&, EntryType type) const;

        virtual void path_changed() {}
        virtual void entry_hoverred(const Entry*) {}
        virtual void entry_clicked(const Entry& entry);
        virtual bool allow_modify() const;

    protected:
        void on_gui() override;

    private:
        bool process_context_menu();
        void make_drop_target(std::string_view drop_path);

        const FileSystemModel* _filesystem = nullptr;

        bool _at_root = true;
        bool _refresh = false;

        usize _hovered = usize(-1);
        core::Vector<Entry> _entries;

        core::String _current_path;

        static constexpr auto update_duration = core::Duration::seconds(1.0);

        core::Chrono _update_chrono;
};

}

#endif // EDITOR_WIDGETS_FILESYSTEMVIEW_H

