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
#ifndef EDITOR_RENDERERS_IMGUIRENDERER_H
#define EDITOR_RENDERERS_IMGUIRENDERER_H

#include <editor/editor.h>

#include <yave/graphics/buffers/buffers.h>
#include <yave/material/Material.h>

namespace editor {

class ImGuiRenderer : NonCopyable, public ContextLinked {

    struct Vertex {
        const math::Vec2 pos;
        const math::Vec2 uv;
        const u32 col;
    };

    Y_TODO(Merge ImGuiRenderer into Ui)

    public:
        enum class Style {
            Yave,
            Yave2,
            Corporate,
            Corporate3D,
            Blender
        };

        ImGuiRenderer(ContextPtr ctx);

        void render(RenderPassRecorder& recorder, const FrameToken&);

        void set_style(Style st);

        const Texture& font_texture() const;

    private:
        Texture _font;
        TextureView _font_view;
};

}

#endif // EDITOR_RENDERERS_IMGUIRENDERER_H

