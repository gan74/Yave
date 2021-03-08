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

#include "UiManager.h"

#include <editor/widgets/EngineView.h>
#include <editor/widgets/ResourceBrowser.h>
#include <editor/widgets/FileBrowser.h>

#include <yave/utils/FileSystemModel.h>

#include <y/core/HashMap.h>

#include <external/imgui/yave_imgui.h>

namespace editor {

UiDebugWidget::UiDebugWidget() : Widget("UI Debug") {
}

void UiDebugWidget::on_gui() {
    if(ImGui::Button("Add test widget")) {
        add_widget(std::make_unique<Widget>("Test widget"), true);
    }
    if(ImGui::Button("Add resource browser")) {
        add_widget(std::make_unique<ResourceBrowser>(), true);
    }
    if(ImGui::Button("Add file explorer")) {
        add_widget(std::make_unique<FileBrowser>(FileSystemModel::local_filesystem()), true);
    }
    if(ImGui::Button("Add engine view")) {
        add_widget(std::make_unique<EngineView>(), true);
    }
}

UiManager::UiManager() {
}

UiManager::~UiManager() {
}

void UiManager::on_gui() {
    core::ExternalHashMap<Widget*, int> to_destroy;

    for(auto& widget : _widgets) {
        y_profile_dyn_zone(widget->_title_with_id);

        _auto_parent = widget.get();
        widget->draw_gui_inside();

        if(!widget->is_visible()) {
            to_destroy[widget.get()];
        }
    }

    _auto_parent = nullptr;


   if(!to_destroy.is_empty()) {
        for(usize i = 0;  i != _widgets.size(); ++i) {
            bool destroy = to_destroy.contains(_widgets[i].get());
            for(Widget* parent = _widgets[i]->_parent; parent && !destroy; parent = parent->_parent) {
                destroy |= to_destroy.contains(parent);
            }

            if(destroy) {
                _widgets.erase_unordered(_widgets.begin() + i);
                --i;
            }
        }
    }
}

Widget* UiManager::add_widget(std::unique_ptr<Widget> widget, bool auto_parent) {
    Widget* wid = widget.get();

    if(auto_parent && _auto_parent) {
        wid->set_parent(_auto_parent);
    }

    set_widget_id(wid);
    _widgets << std::move(widget);

    return wid;
}

void UiManager::set_widget_id(Widget* widget) {
    WidgetIdStack& ids = _ids[typeid(*widget)];
    if(!ids.released.is_empty()) {
        widget->set_id(ids.released.pop());
    } else {
        widget->set_id(++ids.next);
    }
}

}

