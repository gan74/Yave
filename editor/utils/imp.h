/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#ifndef EDITOR_UTILS_IMP_H
#define EDITOR_UTILS_IMP_H

#include <editor/editor.h>

#include <external/imgui/yave_imgui.h>

namespace editor {

namespace imp {

using Str = const core::String&;

struct StyleVar {
    enum class Type {
        Var1D, Var2D, Color,
    };

    Type type;
    union {
        ImGuiStyleVar var;
        ImGuiCol col;
    };

    union {
        float val_1d;
        ImVec2 val_2d;
        u32 val_col;
    };



    StyleVar(ImGuiStyleVar var, float val) : type(Type::Var1D), var(var), val_1d(val) {
    }

    StyleVar(ImGuiStyleVar var, ImVec2 val) : type(Type::Var2D), var(var), val_2d(val) {
    }

    StyleVar(ImGuiCol col, u32 val) : type(Type::Color), col(col), val_col(val) {
    }

    StyleVar(ImGuiCol col, ImVec4 val) : StyleVar(col, ImGui::ColorConvertFloat4ToU32(val)) {
    }


    void apply(int& style_var, int& style_col) const {
        switch(type) {
            case Type::Var1D:
                ++style_var;
                ImGui::PushStyleVar(var, val_1d);
            break;

            case Type::Var2D:
                ++style_var;
                ImGui::PushStyleVar(var, val_2d);
            break;

            case Type::Color:
                ++style_col;
                ImGui::PushStyleColor(col, val_col);
            break;
        }
    }
};


using Style = std::initializer_list<StyleVar>;

struct Obj : NonCopyable {
    public:
        Obj() = default;

        Obj(Style style) {
            for(const auto& var : style) {
                var.apply(_style_var, _style_col);
            }
        }

        ~Obj() {
            if(_style_var) {
                ImGui::PopStyleVar(_style_var);
            }
            if(_style_col) {
                ImGui::PopStyleColor(_style_col);
            }
        }

        Obj(Obj&& other) {
            swap(other);
        }

        Obj& operator=(Obj&& other) {
            swap(other);
            return *this;
        }

        void swap(Obj& other) {
            std::swap(_open, other._open);
            std::swap(_style_var, other._style_var);
            std::swap(_style_col, other._style_col);
        }

        operator bool() const {
            return _open;
        }

        void set(bool open) {
            _open = open;
        }

    private:
        bool _open = true;
        int _style_var = 0;
        int _style_col = 0;

};




Obj text(Str text, Style style = {}) {
    Obj obj(style);
    ImGui::TextUnformatted(text.data(), text.data() + text.size());
    return obj;
}

Obj checkbox(Str text, bool& c, Style style = {}) {
    Obj obj(style);
    obj.set(ImGui::Checkbox(text.data(), &c));
    return obj;
}

Obj header(Str text, Style style = {}) {
    Obj obj(style);
    obj.set(ImGui::CollapsingHeader(text.data()));
    return obj;
}


}

}

#endif // EDITOR_UTILS_IMP_H

