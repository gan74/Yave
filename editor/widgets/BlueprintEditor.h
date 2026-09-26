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
#ifndef EDITOR_WIDGETS_BLUEPRINTEDITOR_H
#define EDITOR_WIDGETS_BLUEPRINTEDITOR_H

#include <editor/Widget.h>
#include <editor/BlueprintWorkspace.h>

#include <yave/blueprints/Blueprint.h>

#include <y/core/Vector.h>

namespace ax::NodeEditor {
struct EditorContext;
}

namespace editor {

class BlueprintEditor final : public WorkspaceWidget<BlueprintWorkspace> {

    editor_widget_open(BlueprintEditor, Center)

    public:
        BlueprintEditor(BlueprintWorkspace* ws);
        ~BlueprintEditor() override;

    protected:
        void on_gui() override;

    private:
        struct Link {
            u64 id = 0;
            uintptr_t start_pin = 0;
            uintptr_t end_pin = 0;
            BlueprintParamTypeIndex type = BlueprintParamTypeIndex::invalid_index;
        };

        void draw_node(const BlueprintNode& node);
        void process_links();
        void rebuild_blueprint_links();
        bool is_pin_linked(uintptr_t pin) const;

        ax::NodeEditor::EditorContext* _context = nullptr;
        std::unique_ptr<Blueprint> _blueprint;

        core::Vector<Link> _links;
        u64 _next_link_id = 1;
};

}

#endif // EDITOR_WIDGETS_BLUEPRINTEDITOR_H
