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
#ifndef EDITOR_WIDGETS_FILEBROWSER_H
#define EDITOR_WIDGETS_FILEBROWSER_H

#include "FileSystemView.h"

#include <functional>

namespace editor {

class FileBrowser final : public FileSystemView {
    public:
        FileBrowser(const FileSystemModel* filesystem = nullptr);

        template<typename F>
        void set_selected_callback(F&& func) {
            _callbacks.selected = y_fwd(func);
        }

        template<typename F>
        void set_canceled_callback(F&& func) {
            _callbacks.canceled = y_fwd(func);
        }

        void set_selection_filter(bool dirs, std::string_view exts = "");

    protected:
        void on_gui() override;

        void path_changed() override;
        core::Result<core::String> entry_icon(const core::String &name, EntryType type) const override;
        void entry_clicked(const Entry& entry) override;

    private:
        bool has_valid_extension(std::string_view filename) const;
        bool done(const core::String& filename);
        void cancel();

        core::String full_path() const;

        const FileSystemModel* _filesystem = nullptr;

        bool _dirs = false;

        core::Vector<core::String> _extensions;

        static constexpr usize buffer_capacity = 1024;
        std::array<char, buffer_capacity> _path_buffer = {};
        std::array<char, buffer_capacity> _name_buffer = {};

        struct {
            std::function<bool(const core::String&)> selected = [](const auto&) { return false; };
            std::function<bool()> canceled = [] { return true; };
        } _callbacks;
};

}

#endif // EDITOR_WIDGETS_FILEBROWSER_H

