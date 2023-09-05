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

#include "ImGuiRenderer.h"

#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/images/ImageData.h>
#include <yave/graphics/buffers/Buffer.h>
#include <yave/material/Material.h>

#include <y/core/Chrono.h>
#include <y/io2/File.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <external/imgui/fa-solid-900.h>
#include <external/imgui/jetbrains_mono.h>

#include <editor/utils/ui.h>

namespace editor {

static ImageData load_font() {
    y_profile();

    ImFontAtlas* fonts = ImGui::GetIO().Fonts;

    {
        fonts->AddFontFromMemoryCompressedTTF(jetbrains_mono_compressed_data, font_awesome_compressed_size, 15.0f);
    }

    {
        ImFontConfig config;
        config.MergeMode = true;
        config.PixelSnapH = true;
        config.OversampleV = 1;
        config.OversampleH = 1;

        const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
        fonts->AddFontFromMemoryCompressedTTF(font_awesome_compressed_data, font_awesome_compressed_size, 13.0f, &config, icon_ranges);
    }

    u8* font_data = nullptr;
    int width = 0;
    int height = 0;
    fonts->GetTexDataAsRGBA32(&font_data, &width, &height);
    return ImageData(math::Vec2ui(width, height), font_data, ImageFormat(VK_FORMAT_R8G8B8A8_UNORM));
}

static MaterialTemplateData create_imgui_material_data() {
    return MaterialTemplateData()
            .set_frag_data(SpirVData::deserialized(io2::File::open("imgui.frag.spv").expected("Unable to open SPIR-V file.")))
            .set_vert_data(SpirVData::deserialized(io2::File::open("imgui.vert.spv").expected("Unable to open SPIR-V file.")))
            .set_depth_mode(DepthTestMode::None)
            .set_cull_mode(CullMode::None)
            .set_blend_mode(BlendMode::SrcAlpha)
        ;
}


ImGuiRenderer::ImGuiRenderer() :
        _font(load_font()),
        _font_view(_font),
        _material(create_imgui_material_data()) {
}


const Texture& ImGuiRenderer::font_texture() const {
    return _font;
}

void ImGuiRenderer::render(ImDrawData* draw_data, RenderPassRecorder& recorder) {
    static_assert(sizeof(ImDrawIdx) == sizeof(u32), "16 bit indices not supported");

    y_profile();

    if(!draw_data) {
        return;
    }

    const auto region = recorder.region("ImGui render", nullptr, math::Vec4(0.7f, 0.7f, 0.7f, 1.0f));

    const auto next_power_of_2 = [](usize size) { return 2 << log2ui(size); };
    const usize imgui_index_buffer_size = next_power_of_2(draw_data->TotalIdxCount);
    const usize imgui_vertex_buffer_size = next_power_of_2(draw_data->TotalVtxCount);
    const math::Vec2 viewport_size = recorder.viewport().extent;
    const math::Vec2 viewport_offset = draw_data->DisplayPos;


    const TypedBuffer<u32, BufferUsage::IndexBit, MemoryType::CpuVisible> index_buffer(imgui_index_buffer_size);
    const TypedBuffer<ImDrawVert, BufferUsage::AttributeBit, MemoryType::CpuVisible> vertex_buffer(imgui_vertex_buffer_size);
    const TypedUniformBuffer<math::Vec2, MemoryType::CpuVisible> uniform_buffer(2);

    auto indices = index_buffer.map(MappingAccess::WriteOnly);
    auto vertices = vertex_buffer.map(MappingAccess::WriteOnly);
    auto uniform = uniform_buffer.map(MappingAccess::WriteOnly);

    uniform[0] = viewport_size;
    uniform[1] = viewport_offset;

    const auto create_descriptor_set = [&](const TextureView* tex) {
        return DescriptorSet(Descriptor(*tex, SamplerType::LinearClamp), uniform_buffer);
    };

    const DescriptorSetBase default_set = create_descriptor_set(&_font_view);

    const auto setup_state = [&](const TextureView* tex) {
        recorder.bind_material_template(&_material, tex ? create_descriptor_set(tex) : default_set);
    };

    usize index_offset = 0;
    usize vertex_offset = 0;
    ImTextureID current_tex = 0;

    setup_state(nullptr);

    recorder.bind_index_buffer(index_buffer);
    recorder.bind_attrib_buffers({vertex_buffer});
    for(auto c = 0; c != draw_data->CmdListsCount; ++c) {
        Y_TODO(Use vertex offsets so we can enable ImGuiBackendFlags_RendererHasVtxOffset)

        const ImDrawList* cmd_list = draw_data->CmdLists[c];

        if(cmd_list->IdxBuffer.Size + index_offset >= index_buffer.size()) {
            y_fatal("Index buffer overflow.");
        }

        if(cmd_list->VtxBuffer.Size + vertex_offset >= vertex_buffer.size()) {
            y_fatal("Vertex buffer overflow.");
        }

        std::copy(cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Data + cmd_list->IdxBuffer.Size, &indices[index_offset]);
        std::copy(cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Data + cmd_list->VtxBuffer.Size, &vertices[vertex_offset]);

        usize drawn_index_offset = index_offset;
        for(auto i = 0; i != cmd_list->CmdBuffer.Size; ++i) {
            const ImDrawCmd& cmd = cmd_list->CmdBuffer[i];

            const math::Vec2i offset = math::Vec2i(i32(cmd.ClipRect.x), i32(cmd.ClipRect.y)) - math::Vec2i(viewport_offset);
            const math::Vec2ui extent(u32(cmd.ClipRect.z - cmd.ClipRect.x), u32(cmd.ClipRect.w - cmd.ClipRect.y));
            recorder.set_scissor(offset.max(math::Vec2(0.0f)), extent);

            y_always_assert(!cmd.UserCallback, "User callback not supported");

            if(cmd.ElemCount) {
                if(current_tex != cmd.TextureId) {
                    current_tex = cmd.TextureId;
                    setup_state(UiTexture::view(current_tex));
                }

                VkDrawIndexedIndirectCommand command = {};
                {
                    command.firstIndex = u32(drawn_index_offset);
                    command.vertexOffset = u32(vertex_offset);
                    command.indexCount = cmd.ElemCount;
                    command.instanceCount = 1;
                }
                recorder.draw(command);

                drawn_index_offset += cmd.ElemCount;
            }
        }

        vertex_offset += cmd_list->VtxBuffer.Size;
        index_offset += cmd_list->IdxBuffer.Size;
    }
}


}

