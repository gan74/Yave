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
#ifndef EDITOR_WIDGETS_GLTFIMPORTER_H
#define EDITOR_WIDGETS_GLTFIMPORTER_H

#include "FileBrowser.h"

#include <editor/import/import.h>

#include <y/concurrent/JobSystem.h>

namespace editor {

class GltfImporter final : public Widget {

    enum class State {
        Browsing,
        Parsing,
        Settings,
        Importing,
        Done,
        Failed,
    };

    public:
        GltfImporter();
        GltfImporter(core::String dst_import_path);

    protected:
        void on_gui() override;

        bool should_keep_alive() const override;

    private:
        State _state = State::Browsing;

        FileBrowser _browser;
        core::Result<import::ParsedScene> _scene = core::Err();

        struct {
            core::String import_path;
            bool import_child_prefabs_as_assets = false;
            bool create_colliders = false;
        } _settings;


        concurrent::JobSystem _job_system;
};

}

#endif // EDITOR_WIDGETS_GLTFIMPORTER_H

