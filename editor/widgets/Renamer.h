/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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
#ifndef EDITOR_WIDGETS_RENAMER_H
#define EDITOR_WIDGETS_RENAMER_H

#include <editor/Widget.h>

#include <y/core/FixedArray.h>

#include <functional>

namespace editor {

class Renamer : public Widget {
    public:
        Renamer(core::String name, std::function<bool(std::string_view)> callback);

        const core::String& original_name() const;

    protected:
        void on_gui() override;

    private:
        core::String _name;
        core::FixedArray<char> _name_buffer;

        std::function<bool(std::string_view)> _callback;
};

class FileRenamer final : public Renamer {
    public:
        FileRenamer(const FileSystemModel* fs, core::String filename);
};

}

#endif // EDITOR_WIDGETS_FILERENAMER_H

