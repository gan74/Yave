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
#ifndef EDITOR_WIDGETS_IMAGEIMPORTER_H
#define EDITOR_WIDGETS_IMAGEIMPORTER_H

#include "FileBrowser.h"

#include <yave/graphics/images/ImageData.h>

#include <editor/utils/Named.h>

#include <future>

namespace editor {

class ImageImporter final : public Widget {

    public:
        ImageImporter();
        ImageImporter(const core::String& import_path);

    protected:
        void on_gui() override;

    private:
        void import_async(const core::String& filename);
        void import(const Named<ImageData>& asset);

        bool done_loading() const;
        bool is_loading() const;

        FileBrowser _browser;

        core::String _import_path;

        std::future<Named<ImageData>> _import_future;
};

}

#endif // EDITOR_WIDGETS_IMAGEIMPORTER_H

