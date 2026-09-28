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
#ifndef EDITOR_WIDGETS_BLUEPRINTNODEINSPECTOR_H
#define EDITOR_WIDGETS_BLUEPRINTNODEINSPECTOR_H

#include <editor/Widget.h>
#include <editor/BlueprintWorkspace.h>

#include <y/core/HashMap.h>

#include <functional>
#include <string_view>

namespace editor {

class BlueprintNodeInspector final : public WorkspaceWidget<BlueprintWorkspace> {

    editor_widget_open(BlueprintNodeInspector, Right)

    public:
        BlueprintNodeInspector(BlueprintWorkspace* ws);

    protected:
        void on_gui() override;

    private:
        struct ParamDrawer {
            std::function<void(std::string_view, void*)> input;
            std::function<void(std::string_view, const void*)> output;
        };

        core::FlatHashMap<const BlueprintParamType*, ParamDrawer> _drawers;
};

}

#endif // EDITOR_WIDGETS_BLUEPRINTNODEINSPECTOR_H
