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

#include <editor/Widget.h>
#include <editor/EditorWorld.h>
#include <editor/utils/memory.h>
#include <editor/ThumbmailRenderer.h>

#include <yave/scene/SceneView.h>
#include <yave/graphics/device/MeshAllocator.h>
#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>

#include <y/core/Chrono.h>

#include <editor/utils/ui.h>

#include <y/utils/format.h>

#include <yave/ecs/System.h>


namespace editor {

class CameraDebug : public Widget {
    editor_widget(CameraDebug, "View", "Debug")

    public:
        CameraDebug() : Widget(ICON_FA_VIDEO " Camera debug", ImGuiWindowFlags_AlwaysAutoResize) {
        }

    protected:
        void on_gui() override {
            const Camera& camera = scene_view().camera();
            const math::Vec3 pos = camera.position();
            const math::Vec3 fwd = camera.forward();
            const math::Vec3 rht = camera.right();
            const math::Vec3 up = fwd.cross(rht);

            const math::Quaternion<> rot = math::Quaternion<>::from_base(fwd, rht, up);

            ImGui::Text("FoV: %.1f", camera.field_of_view());
            ImGui::Text("Aspect ratio: %.2f", camera.aspect_ratio());

            ImGui::Separator();

            ImGui::Text("position: %.1f, %.1f, %.1f", pos.x(), pos.y(), pos.z());
            ImGui::Text("forward : %.1f, %.1f, %.1f", fwd.x(), fwd.y(), fwd.z());
            ImGui::Text("right   : %.1f, %.1f, %.1f", rht.x(), rht.y(), rht.z());
            ImGui::Text("up      : %.1f, %.1f, %.1f", up.x(), up.y(), up.z());

            ImGui::Text("rotation: %.1f, %.1f, %.1f, %.1f", rot.x(), rot.y(), rot.z(), rot.w());

            if(ImGui::CollapsingHeader("Rotation")) {
                const math::Vec3 x = rot({1.0f, 0.0f, 0.0f});
                const math::Vec3 y = rot({0.0f, 1.0f, 0.0f});
                const math::Vec3 z = rot({0.0f, 0.0f, 1.0f});
                ImGui::Text("X axis: %.1f, %.1f, %.1f", x.x(), x.y(), x.z());
                ImGui::Text("Y axis: %.1f, %.1f, %.1f", y.x(), y.y(), y.z());
                ImGui::Text("Z axis: %.1f, %.1f, %.1f", z.x(), z.y(), z.z());

                ImGui::Separator();

                auto euler = rot.to_euler();
                ImGui::Text("pitch: %.1f°", math::to_deg(euler[math::Quaternion<>::PitchIndex]));
                ImGui::Text("yaw  : %.1f°", math::to_deg(euler[math::Quaternion<>::YawIndex]));
                ImGui::Text("roll : %.1f°", math::to_deg(euler[math::Quaternion<>::RollIndex]));
            }
        }
};

class MemoryDebug : public Widget {
    editor_widget(MemoryDebug, "View", "Debug")

    public:
        MemoryDebug() : Widget("Memory debug", ImGuiWindowFlags_AlwaysAutoResize) {
        }

    protected:
        void on_gui() override {
            const u64 total_allocs = memory::total_allocations();
            ImGui::TextUnformatted(fmt_c_str("{} live allocations", memory::live_allocations()));
            ImGui::TextUnformatted(fmt_c_str("{} allocations per frame", total_allocs - _last_total));
            _last_total = total_allocs;
        }

    private:
        u64 _last_total = 0;
};


class EcsDebug : public Widget {
    editor_widget(EcsDebug, "View", "Debug")

    public:
        EcsDebug() : Widget("ECS Debug") {
        }

    protected:
        void on_gui() override {
            EditorWorld& world = current_world();

            const ImGuiTableFlags table_flags = ImGuiTableFlags_RowBg;


            if(ImGui::CollapsingHeader("Groups")) {
                const auto groups = world.all_groups();
                for(const ecs::EntityGroupBase* group : groups) {
                    ImGui::TextUnformatted(group->name().data());
                }
            }

            if(ImGui::CollapsingHeader("Tags")) {
                if(ImGui::BeginTable("##tags", 3, table_flags)) {
                    ImGui::TableSetupColumn("##tag", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("##entities", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("##actions",ImGuiTableColumnFlags_WidthFixed);

                    for(const core::String& tag : world.tags()) {
                        imgui::table_begin_next_row();
                        ImGui::TextUnformatted(tag.data());
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(fmt_c_str("{} entities", world.tag_set(tag)->size()));
                        ImGui::TableNextColumn();
                        if(ImGui::SmallButton(ICON_FA_TRASH)) {
                            world.clear_tag(tag);
                        }
                    }
                    ImGui::EndTable();
                }
            }
        }
};


class MeshAllocatorDebug : public Widget {
    editor_widget(MeshAllocatorDebug, "View", "Debug")

    public:
        MeshAllocatorDebug() : Widget("Mesh allocator debug") {
        }

    protected:
        void on_gui() override {
            auto [vert, tris] = mesh_allocator().allocated();

            {
                ImGui::TextUnformatted("Vertex buffer:");
                ImGui::SameLine();
                ImGui::ProgressBar(float(vert) / MeshAllocator::default_vertex_count, ImVec2(-1.0f, 0.0f),
                    fmt_c_str("{}k / {}k", vert / 1000, MeshAllocator::default_vertex_count / 1000)
                );
            }
            {
                ImGui::TextUnformatted("Triangle buffer:");
                ImGui::SameLine();
                ImGui::ProgressBar(float(tris) / MeshAllocator::default_triangle_count, ImVec2(-1.0f, 0.0f),
                    fmt_c_str("{}k / {}k", tris / 1000, MeshAllocator::default_triangle_count / 1000)
                );
            }
        }
};


class SelectionDebug : public Widget {
    editor_widget(SelectionDebug, "View", "Debug")

    public:
        SelectionDebug() : Widget("Selection debug") {
        }

    protected:
        void on_gui() override {
            if(ImGui::CollapsingHeader(fmt_c_str("{} entity selected###header", current_world().selected_entities().size()), ImGuiTreeNodeFlags_DefaultOpen)) {
                for(const ecs::EntityId id : current_world().selected_entities()) {
                    imgui::text_read_only(fmt_c_str("##{}", id.index()), fmt("{:#08x}", id.index()));
                    ImGui::SameLine();
                    const auto name = current_world().entity_name(id);
                    ImGui::TextUnformatted(name.data(), name.data() + name.size());
                }
            }
        }
};


class UiDebug : public Widget {
    editor_widget(UiDebug, "View", "Debug")

    public:
        UiDebug() : Widget("UI debug") {
        }

    protected:
        void on_gui() override {
            ImGui::Text("%u cached thumbmails", u32(thumbmail_renderer().cached_thumbmails()));
        }
};

}
