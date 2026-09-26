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
#ifndef EDITOR_BLUEPRINTWORKSPACE_H
#define EDITOR_BLUEPRINTWORKSPACE_H

#include "Workspace.h"

#include <yave/blueprints/Blueprint.h>

namespace editor {

class BlueprintWorkspace final : public Workspace {
    public:
        BlueprintWorkspace();
        ~BlueprintWorkspace() override;

        void update() override;

        std::string_view name() const override;

        void save() override;
        void load() override;

        Blueprint& blueprint();
        const Blueprint& blueprint() const;

        BlueprintNode* selected_node() const;
        void set_selected_node(BlueprintNode* node);

    private:
        Blueprint _blueprint;
        BlueprintNode* _selected_node = nullptr;
};

}

#endif // EDITOR_BLUEPRINTWORKSPACE_H
