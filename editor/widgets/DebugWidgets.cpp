/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
#include <editor/ThumbnailRenderer.h>
#include <editor/systems/UndoRedoSystem.h>
#include <editor/components/EditorComponent.h>

#include <yave/scene/SceneView.h>
#include <yave/graphics/device/MeshAllocator.h>
#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/renderer/SceneVisibilitySubPass.h>

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


class CullingDebug : public Widget {
    editor_widget(CullingDebug, "View", "Debug")

    public:
        CullingDebug() : Widget("Culling debug", ImGuiWindowFlags_AlwaysAutoResize) {
        }

    protected:
        void on_gui() override {
            const core::StopWatch timer;
            const SceneVisibilitySubPass visibility = SceneVisibilitySubPass::create(scene_view());
            const core::Duration durr = timer.elapsed();

            ImGui::TextUnformatted("Visible for current camera:");
            ImGui::TextUnformatted(fmt_c_str("{} meshes", visibility.visible->meshes.size()));
            ImGui::TextUnformatted(fmt_c_str("{} point lights", visibility.visible->point_lights.size()));
            ImGui::TextUnformatted(fmt_c_str("{} spot lights", visibility.visible->spot_lights.size()));

            ImGui::Separator();

            ImGui::TextUnformatted(fmt_c_str("visibility time: {:.2}ms", durr.to_millis()));
        }
};


static void busy_sleep(core::Duration dur) {
    core::StopWatch timer;
    while(timer.elapsed() < dur) {
        std::this_thread::yield();
    }
}

struct TestSystem : ecs::System {
    TestSystem() : ecs::System("Test") {
    }

    void setup(ecs::SystemScheduler& sched) {
        sched.schedule(ecs::SystemSchedule::Tick, "Tick", [] {
            busy_sleep(core::Duration::milliseconds(0.5));
        });
        sched.schedule(ecs::SystemSchedule::Update, "Update", [] {
            busy_sleep(core::Duration::milliseconds(0.5));
        });
        sched.schedule(ecs::SystemSchedule::PostUpdate, "Post", [] {
            busy_sleep(core::Duration::milliseconds(0.5));
        });
    }
};

class EcsDebug : public Widget {
    editor_widget(EcsDebug, "View", "Debug")

    public:
        EcsDebug() : Widget("ECS debug") {
        }

    protected:
        void on_gui() override {
            EditorWorld& world = current_world();


            if(!world.find_system<TestSystem>()){
                world.add_system<TestSystem>();
            }

            const ImGuiTableFlags table_flags = ImGuiTableFlags_RowBg;

            if(ImGui::CollapsingHeader("Groups")) {
                auto groups = world.group_providers();
                std::sort(groups.begin(), groups.end(), [](auto* a, auto* b) { return a->name() < b->name(); });
                for(const ecs::EntityGroupProvider* group : groups) {
                    if(ImGui::TreeNode(fmt_c_str("{}###{}", group->name(), static_cast<const void*>(group)))) {
                        ImGui::TextUnformatted(fmt_c_str("{} ids", group->ids().size()));

                        if(!group->tags().is_empty()) {
                            if(ImGui::TreeNode(fmt_c_str("{} tags", group->tags().size()))) {
                                for(const core::String& tag : group->tags()) {
                                    ImGui::TextUnformatted(tag.data());
                                }
                                ImGui::TreePop();
                            }
                        }

                        if(!group->type_filters().is_empty()) {
                            if(ImGui::TreeNode(fmt_c_str("{} type filters", group->type_filters().size()))) {
                                for(const ecs::ComponentTypeIndex type : group->type_filters()) {
                                    ImGui::TextUnformatted(fmt_c_str("{}", world.component_type_name(type)));
                                }
                                ImGui::TreePop();
                            }
                        }

                        ImGui::TreePop();
                    }
                }
            }

            if(ImGui::CollapsingHeader("Containers")) {
                for(const ecs::ComponentContainerBase* cont : world.component_containers()) {
                    const std::string_view name = cont->runtime_info().clean_component_name();
                    ImGui::TextUnformatted(name.data(), name.data() + name.size());
                }
            }

            if(ImGui::CollapsingHeader("Tags")) {
                if(ImGui::BeginTable("##tags", 3, table_flags)) {
                    ImGui::TableSetupColumn("##tag", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("##entities", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("##actions",ImGuiTableColumnFlags_WidthFixed);

                    for(const core::String& tag : world.tags()) {
                        ImGui::PushID(tag.data());
                        imgui::table_begin_next_row();
                        ImGui::TextUnformatted(tag.data());
                        ImGui::TableNextColumn();

                        ImGui::TextUnformatted(fmt_c_str("{} entities", world.tag_set(tag)->size()));

                        ImGui::TableNextColumn();
                        if(ImGui::SmallButton(ICON_FA_TRASH)) {
                            world.clear_tag(tag);
                            y_debug_assert(world.tag_set(tag)->is_empty());
                        }
                        ImGui::PopID();
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

            {
                auto tris = mesh_allocator().allocated();
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
            ThumbnailRenderer& tb = thumbnail_renderer();
            ImGui::TextUnformatted(fmt_c_str("{} cached thumbnails", tb.cached_thumbnails()));
            ImGui::Separator();
            ImGui::TextUnformatted(fmt_c_str("Thumbnail size: {}x{}", tb.thumbnail_size, tb.thumbnail_size));
            ImGui::TextUnformatted(fmt_c_str("Total thumbnail memory: {}KB", (tb.cached_thumbnails() * tb.thumbnail_size * tb.thumbnail_size * 4) / 1024));
        }
};


class DebugValueEditor : public Widget {
    editor_widget(DebugValueEditor, "View", "Debug")

    public:
        DebugValueEditor() : Widget("Debug values") {
        }


    protected:
        void on_gui() override {
            DebugValues& values = debug_values();

            for(usize i = 0; i != 4; ++i) {
                if(i) {
                    ImGui::SameLine();
                }
                ImGui::Checkbox(fmt_c_str("Bool (f) #{}", i), &values.bool_f[i]);
            }

            for(usize i = 0; i != 4; ++i) {
                if(i) {
                    ImGui::SameLine();
                }
                ImGui::Checkbox(fmt_c_str("Bool (t) #{}", i), &values.bool_t[i]);
            }

            for(usize i = 0; i != 4; ++i) {
                ImGui::InputFloat(fmt_c_str("Float (0.0) #{}", i), &values.float_0[i]);
            }

            for(usize i = 0; i != 4; ++i) {
                ImGui::InputFloat(fmt_c_str("Float (1.0) #{}", i), &values.float_1[i]);
            }
        }
};


class RaytracingDebug : public Widget {
    editor_widget(RaytracingDebug, "View", "Debug")

    public:
        RaytracingDebug() : Widget("Raytracing debug") {
        }

    protected:
        void on_gui() override {
            if(!raytracing_enabled()) {
                ImGui::TextColored(imgui::error_text_color, ICON_FA_EXCLAMATION_TRIANGLE " Ray-tracing disabled");
                return;
            }

            const TLAS& tlas = current_scene().tlas();
            ImGui::Text("TLAS size = %uKB", u32(tlas.buffer().byte_size() / 1024));
        }
};


class UndoRedoDebug : public Widget {

    editor_widget(UndoRedoDebug)

    public:
        UndoRedoDebug() : Widget("Undo Redo") {
        }

        void on_gui() override {
            UndoRedoSystem* system = current_world().find_system<UndoRedoSystem>();
            const auto states = system->undo_states();

            ImGui::Text("%u items in stack (current: %u)", u32(states.size()), u32(system->stack_top()));

            ImGui::SameLine();

            if(ImGui::Button("Clear")) {
                system->reset();
            }

            if(ImGui::BeginChild("##undostack")) {
                for(usize i = 0; i != states.size(); ++i) {
                    const bool current = (i == system->stack_top() - 1);
                    if(ImGui::TreeNodeEx(fmt_c_str("State #{}", i), ImGuiTreeNodeFlags_SpanAvailWidth | (current ? ImGuiTreeNodeFlags_Selected : 0)))  {
                        ImGui::Indent();

                        for(const auto& [id, data] : states[i].removed_components) {
                            const auto type_name = current_world().component_type_name(data.type_id);
                            ImGui::TextUnformatted(fmt_c_str("({:08x}:{:08x}).{} deleted", id.index(), id.version(), type_name));
                        }

                        for(const auto& [id, data] : states[i].added_components) {
                            const auto type_name = current_world().component_type_name(data.type_id);
                            ImGui::TextUnformatted(fmt_c_str("({:08x}:{:08x}).{} added", id.index(), id.version(), type_name));
                        }

                        for(const auto& [key, props] : states[i].undo_properties) {
                            const auto type_name = current_world().component_type_name(key.second);
                            for(const auto& prop : props) {
                                ImGui::TextUnformatted(fmt_c_str("({:08x}:{:08x}).{}.{} changed", key.first.index(), key.first.version(), type_name, prop.name));
                            }
                        }

                        ImGui::Unindent();
                        ImGui::TreePop();
                    }
                }
            }

            ImGui::EndChild();
        }
};


class VisibilityDebug : public Widget {

    editor_widget(VisibilityDebug)

    public:
        VisibilityDebug() : Widget("Visibility debug") {
        }

        void on_gui() override {
            const std::shared_ptr<SceneVisibility> visible = SceneVisibilitySubPass::create(scene_view()).visible;
            ImGui::TextUnformatted(fmt_c_str("{} visible meshes", visible->meshes.size()));
            ImGui::TextUnformatted(fmt_c_str("{} visible point lights", visible->point_lights.size()));
            ImGui::TextUnformatted(fmt_c_str("{} visible spot lights", visible->spot_lights.size()));

            if(ImGui::CollapsingHeader("ECS")) {
                auto row = []<typename T>(std::string_view name) {
                    const usize total = current_world().create_group<T>().size();
                    const usize hidden = current_world().create_group<T>(ecs::tags::hidden).size();
                    ImGui::TextUnformatted(fmt_c_str("{} {} ({} hidden)", total, name, hidden));
                };
                row.template operator()<StaticMeshComponent>("meshes");
                row.template operator()<PointLightComponent>("point lights");
                row.template operator()<SpotLightComponent>("spot lights");
            }
        }
};

}
