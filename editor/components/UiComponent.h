/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#ifndef EDITOR_COMPONENTS_UICOMPONENT_H
#define EDITOR_COMPONENTS_UICOMPONENT_H

#include <editor/editor.h>

#include <editor/ui/UiElement.h>

#include <yave/ecs/ecs.h>

#include <y/serde3/serde.h>

namespace editor {

struct UiWidget : NonMovable {
    enum Flags {
        None        = 0,
        ShouldClose = 0x01,
    };

    public:
        UiWidget(std::string_view title) {
            set_title(title);
        }

        virtual ~UiWidget() {
        }


        void set_title(std::string_view title) {
            Y_TODO(we need to use an other id to keep imguis setting stack)
            const core::String new_title = fmt("%##%", title, _entity_id.index());
            _title_with_id = std::move(new_title);
            _title = std::string_view(_title_with_id.begin(), title.size());
        }


        virtual void paint(ecs::EntityWorld&, CmdBufferRecorder&) = 0;

    private:
        friend class UiSystem;

        ecs::EntityId _entity_id;
        Flags _flags = None;

        core::String _title_with_id;
        std::string_view _title;

};

class UiComponent {
    public:
        Y_TODO(Fix non serializable components breaking everything)
        y_reflect(parent)

        std::unique_ptr<UiWidget> widget;
        ecs::EntityId parent;
};

}

#endif // EDITOR_COMPONENTS_UICOMPONENT_H

