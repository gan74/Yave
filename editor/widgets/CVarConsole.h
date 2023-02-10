/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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
#ifndef EDITOR_WIDGETS_CVARCONSOLE_H
#define EDITOR_WIDGETS_CVARCONSOLE_H

#include <editor/Widget.h>

#include <y/core/FixedArray.h>
#include <y/core/Vector.h>

#include <functional>

namespace editor {

class CVarConsole : public Widget {

    editor_widget(CVarConsole)

    enum class Status {
        None,
        Error,
        Modified
    };

    struct CVar {
        core::String full_name;
        std::function<std::string_view()> to_string;
        std::function<bool(std::string_view)> from_string;

        core::String type_name;
        core::String default_value;
        bool modified = false;
        bool error = false;
    };

    public:
        CVarConsole();


    protected:
        void on_gui() override;

    private:
        core::FixedArray<char> _search_pattern = core::FixedArray<char>(512);
        core::FixedArray<char> _value = core::FixedArray<char>(512);
        core::Vector<CVar> _cvars;
};

}

#endif // EDITOR_WIDGETS_CVARCONSOLE_H

