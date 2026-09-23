/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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
#ifndef EDITOR_WORKSPACE_H
#define EDITOR_WORKSPACE_H

#include <editor/editor.h>

namespace editor {

class Workspace : NonMovable {
    public:
        Workspace();
        virtual ~Workspace();

        u32 workspace_id() const;

        virtual std::string_view name() const = 0;

        virtual void update() = 0;
        virtual void post_update();

        virtual void grab_reloaded();

        virtual void save() = 0;
        virtual void load() = 0;


    private:
        const u32 _id;
};


class EmptyWorkspace : public Workspace {
    
    public:
        std::string_view name() const override { return "Empty"; }
        void update() override {}

        void save() override {}
        void load() override {}
};

}

#endif // EDITOR_WORKSPACE_H
