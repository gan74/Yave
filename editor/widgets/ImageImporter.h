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
#ifndef EDITOR_WIDGETS_IMAGEIMPORTER_H
#define EDITOR_WIDGETS_IMAGEIMPORTER_H

#include "FileBrowser.h"

#include <yave/graphics/images/ImageData.h>

#include <y/concurrent/StaticThreadPool.h>

namespace editor {

class ImageImporter final : public Widget {

    enum class State {
        Browsing,
        Importing,
        Done
    };

    public:
        ImageImporter();
        ImageImporter(core::String dst_import_path);

        ~ImageImporter();

    protected:
        void on_gui() override;

        bool should_keep_alive() const override;

    private:
        void import(const core::String& filename);

        FileBrowser _browser;

        core::String _import_path;

        State _state = State::Browsing;
        concurrent::WorkerThread _thread_pool;
};

}

#endif // EDITOR_WIDGETS_IMAGEIMPORTER_H

