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

#include "Inspector.h"

#include <editor/EditorWorld.h>
#include <editor/widgets/AssetSelector.h>
#include <editor/widgets/EntitySelector.h>
#include <editor/components/EditorComponent.h>
#include <editor/utils/assets.h>
#include <editor/utils/ui.h>

#include <yave/ecs/ComponentInspector.h>
#include <yave/assets/AssetLoader.h>
#include <yave/graphics/images/IBLProbe.h>
#include <yave/graphics/images/ImageData.h>
#include <yave/material/Material.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/meshes/MeshData.h>

#include <yave/utils/color.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace editor {

[[maybe_unused]]
static core::String clean_type_name(std::string_view name) {
    core::String cleaned;
    cleaned.set_min_capacity(name.size() * 2);
    for(usize i = 0; i != name.size(); ++i) {
        const char c = name[i];
        if(!i) {
            cleaned.push_back(char(std::toupper(c)));
        } else if(std::isupper(c)) {
            cleaned.push_back(' ');
            cleaned.push_back(char(std::tolower(c)));
        } else {
            cleaned.push_back(c);
        }
    }
    return cleaned;
}

class SetterInspectorBase : public ecs::ComponentInspector {
    public:
        SetterInspectorBase(const core::String& name, ecs::ComponentTypeIndex type) :
                _name(name),
                _type(type) {
        }

        bool inspect_component_type(ecs::ComponentRuntimeInfo info, bool) override {
            return _is_type = (info.type_id == _type);
        }

        void inspect(const core::String&, math::Transform<>&)                           override {}
        void inspect(const core::String&, math::Vec3&, Vec3Role)                        override {}
        void inspect(const core::String&, float&, FloatRole)                            override {}
        void inspect(const core::String&, float&, float, float, FloatRole)              override {}
        void inspect(const core::String&, u32&, u32)                                    override {}
        void inspect(const core::String&, bool&)                                        override {}
        void inspect(const core::String&, GenericAssetPtr&)                             override {}
        void inspect(const core::String&, ecs::EntityId&, ecs::ComponentTypeIndex)      override {}

    protected:
        bool is_property(const core::String& name) const {
            return _is_type && name == _name;
        }

    private:
        core::String _name;
        ecs::ComponentTypeIndex _type = {};
        bool _is_type = false;
};

static void id_selector(ecs::EntityId& property_id, const core::String& name, ecs::EntityId entity_id, ecs::ComponentTypeIndex comp_type, ecs::ComponentTypeIndex target_type) {
    class IdSetterInspector : public SetterInspectorBase {
        public:
            IdSetterInspector(ecs::EntityId id, const core::String& name, ecs::ComponentTypeIndex type) :
                    SetterInspectorBase(name, type), _id(id) {
            }

            void inspect(const core::String& name, ecs::EntityId& id, ecs::ComponentTypeIndex) override {
                if(is_property(name)) {
                    id = _id;
                }
            }

        private:
            ecs::EntityId _id;
    };

    bool browse = false;
    imgui::id_selector(property_id, current_world(), target_type, &browse);
    if(browse) {
        add_child_widget<EntitySelector>(target_type)->set_selected_callback(
            [=](ecs::EntityId new_id) {
                IdSetterInspector setter(new_id, name, comp_type);
                current_world().inspect_components(entity_id, &setter);
                return true;
            }
        );
    }
}

template<typename T>
static void asset_ptr_selector(GenericAssetPtr& ptr, const core::String& name, ecs::EntityId id, ecs::ComponentTypeIndex comp_type) {
    class AssetSetterInspector : public SetterInspectorBase {
        public:
            AssetSetterInspector(const AssetPtr<T>& ptr, const core::String& name, ecs::ComponentTypeIndex type) :
                    SetterInspectorBase(name, type), _ptr(ptr) {
            }

            void inspect(const core::String& name, GenericAssetPtr& p) override {
                if(is_property(name)) {
                    p = _ptr;
                }
            }

        private:
            AssetPtr<T> _ptr;
    };

    y_always_assert(ptr.matches<T>(), "AssetPtr doesn't match given type");

    bool clear = false;
    const AssetType type = ptr.type();
    if(imgui::asset_selector(ptr.id(), type, asset_type_name(type, false, false), &clear)) {
        add_child_widget<AssetSelector>(type)->set_selected_callback(
            [=](AssetId asset) {
                if(const auto loaded = asset_loader().load_res<T>(asset)) {
                    AssetSetterInspector setter(loaded.unwrap(), name, comp_type);
                    current_world().inspect_components(id, &setter);
                }
                return true;
            });
    } else if(clear) {
        ptr = {};
    }
}

static const ImGuiTableFlags table_flags =
    ImGuiTableFlags_BordersInnerV |
    // ImGuiTableFlags_BordersInnerH |
    ImGuiTableFlags_Resizable;


static const ImGuiColorEditFlags color_flags =
    ImGuiColorEditFlags_NoSidePreview |
    // ImGuiColorEditFlags_NoSmallPreview |
    ImGuiColorEditFlags_NoAlpha |
    ImGuiColorEditFlags_Float |
    ImGuiColorEditFlags_InputRGB;


static bool begin_property_table() {
    if(ImGui::BeginTable("#properties", 2, table_flags)) {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        return true;
    }
    return false;
}


class InspectorPanelInspector : public ecs::ComponentInspector {
    public:
        InspectorPanelInspector(ecs::EntityId id, EditorComponent* editor, EditorWorld* world) :
                _id(id),
                _editor(editor),
                _world(world) {
        }

        ~InspectorPanelInspector() {
            end_table();
        }

        void end_table() {
            if(_in_table) {
                ImGui::Unindent();
                ImGui::EndTable();
            }
            _in_table = false;
        }

        bool begin_table() {
            y_debug_assert(!_in_table);
            if(begin_property_table()) {
                _in_table = true;
                ImGui::Indent();
            }
            return _in_table;
        }

        bool inspect_component_type(ecs::ComponentRuntimeInfo info, bool has_inspect) override {
            end_table();

            if(info.type_id == ecs::type_index<EditorComponent>()) {
                return false;
            }

            _type = info.type_id;

            bool open = false;
            ImGui::PushID(fmt_c_str("{}", info.type_name));
            {
                const float button_size = ImGui::GetFrameHeight();
                ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - button_size);
                const bool can_remove = !_world->is_component_required(_id, info.type_id);
                const bool remove_component = ImGui::InvisibleButton(ICON_FA_TRASH "###invisible", math::Vec2(button_size));

                ImGui::SameLine();
                ImGui::SetCursorPosX(0.0);
                open = ImGui::CollapsingHeader(fmt_c_str(ICON_FA_PUZZLE_PIECE " {}", info.clean_component_name()), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow);

                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - button_size);
                ImGui::BeginDisabled(!can_remove);
                ImGui::Button(ICON_FA_TRASH, math::Vec2(button_size));
                ImGui::EndDisabled();

                if(remove_component) {
                    if(!can_remove) {
                        log_msg(fmt("{} can not be deleted as it is required by some other component", info.clean_component_name()), Log::Warning);
                    } else {
                        _world->remove_component(_id, info.type_id);
                    }
                }
            }
            ImGui::PopID();

            if(open) {
                if(has_inspect) {
                    begin_table();
                } else {
                    ImGui::Indent();
                    ImGui::TextDisabled("Component is missing inspect()");
                    ImGui::Unindent();
                }
            }

            return _in_table;
        }

        void inspect(const core::String& name, math::Transform<>& tr) override {
            ImGui::PushID(name.data());
            y_defer(ImGui::PopID());

            imgui::table_begin_next_row();

            auto [pos, rot, scale] = tr.decompose();

            // position
            {
                ImGui::TextUnformatted("Position");
                ImGui::TableNextColumn();

                imgui::position_input("##position", pos);
            }

            imgui::table_begin_next_row();

            // rotation
            {
                ImGui::TextUnformatted("Rotation");
                ImGui::TableNextColumn();

                auto is_same_angle = [&](math::Vec3 a, math::Vec3 b) {
                    const auto qa = math::Quaternion<>::from_euler(a);
                    const auto qb = math::Quaternion<>::from_euler(b);
                    for(usize i = 0; i != 3; ++i) {
                        math::Vec3 v;
                        v[i] = 1.0f;
                        if((qa(v) - qb(v)).length2() > 0.001f) {
                            return false;
                        }
                    }
                    return true;
                };

                auto& euler = _editor->euler();

                {
                    math::Vec3 actual_euler = rot.to_euler();
                    if(!is_same_angle(actual_euler, euler)) {
                        euler = actual_euler;
                    }
                }

                auto to_deg = [](math::Vec3 angle) {
                    for(usize i = 0; i != 3; ++i) {
                        float a = math::to_deg(angle[i]);
                        if(a < 0.0f) {
                            a += (std::round(a / -360.0f) + 1.0f) * 360.0f;
                        }
                        y_debug_assert(a >= 0.0f);
                        a = std::fmod(a, 360.0f);
                        y_debug_assert(a < 360.0f);
                        if(a > 180.0f) {
                            a -= 360.0f;
                        }
                        angle[i] = a;
                    }
                    return angle;
                };

                auto to_rad = [](math::Vec3 angle) {
                    for(usize i = 0; i != 3; ++i) {
                        angle[i] = math::to_rad(angle[i]);
                    }
                    return angle;
                };

                math::Vec3 angle = to_deg(euler);
                if(imgui::position_input("##rotation", angle)) {
                    angle = to_rad(angle);
                    euler = angle;
                    rot = math::Quaternion<>::from_euler(angle);
                }
            }

            imgui::table_begin_next_row();

            // scale
            {
                ImGui::TextUnformatted("Scale");
                ImGui::TableNextColumn();

                float scalar_scale = scale.dot(math::Vec3(1.0f / 3.0f));
                if(ImGui::DragFloat("##scale", &scalar_scale, 0.1f, 0.0f, 0.0f, "%.3f")) {
                    scale = std::max(0.001f, scalar_scale);
                }

                const bool is_uniform = (scale.max_component() - scale.min_component()) <= 2 * math::epsilon<float>;
                if(!is_uniform) {
                    ImGui::SameLine();
                    ImGui::TextColored(imgui::error_text_color, ICON_FA_EXCLAMATION_TRIANGLE);
                    if(ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::TextUnformatted(fmt_c_str("Scale is not uniform: {}", scale));
                        ImGui::EndTooltip();
                    }
                }
            }

            tr = math::Transform<>(pos, rot, scale);
        }

        void inspect(const core::String& name, math::Vec3& v, Vec3Role role) override {
            ImGui::PushID(name.data());
            y_defer(ImGui::PopID());

            imgui::table_begin_next_row();
            ImGui::TextUnformatted(name.data());
            ImGui::TableNextColumn();

            switch(role) {

                case Vec3Role::Position:
                    imgui::position_input("##position", v);
                break;

                case Vec3Role::Direction: {
                    float elevation = -math::to_deg(std::asin(v.z()));
                    const math::Vec2 dir_2d = v.to<2>().normalized();
                    float azimuth = math::to_deg(std::copysign(std::acos(dir_2d.x()), std::asin(dir_2d.y())));

                    bool changed = false;
                    changed |= ImGui::DragFloat("Azimuth", &azimuth, 1.0, -180.0f, 180.0f, "%.2f°");
                    changed |= ImGui::DragFloat("Elevation", &elevation, 1.0, -90.0f, 90.0f, "%.2f°");

                    if(changed) {
                        elevation = -math::to_rad(elevation);
                        azimuth = math::to_rad(azimuth);
                        v = math::Vec3(math::Vec2(std::cos(azimuth), std::sin(azimuth)) * std::cos(elevation), std::sin(elevation));
                    }
                } break;

                case Vec3Role::Color: {
                    if(ImGui::ColorButton("Color", math::Vec4(v, 1.0f), color_flags)) {
                        ImGui::OpenPopup("##color");
                    }

                    if(ImGui::BeginPopup("##color")) {
                        ImGui::ColorPicker3("##picker", v.begin(), color_flags);

                        float kelvin = std::clamp(rgb_to_k(v), 1000.0f, 12000.0f);
                        if(ImGui::SliderFloat("##temperature", &kelvin, 1000.0f, 12000.0f, "%.0f°K")) {
                            v = k_to_rbg(kelvin);
                        }

                        ImGui::EndPopup();
                    }
                } break;

                case Vec3Role::None:
                break;
            }

        }

        void inspect(const core::String& name, float& f, float min, float max, FloatRole role) override {
            ImGui::PushID(name.data());
            y_defer(ImGui::PopID());

            imgui::table_begin_next_row();
            ImGui::TextUnformatted(name.data());
            ImGui::TableNextColumn();

            float factor = 1.0f;
            const char* format = "%.4f";
            switch(role) {
                case FloatRole::HalfAngle:
                    factor = math::to_deg(2.0f);
                    format = "%.2f °";
                break;

                case FloatRole::Angle:
                    factor = math::to_deg(1.0f);
                    format = "%.2f °";
                break;

                case FloatRole::Distance:
                    format = "%.2f m";
                break;

                case FloatRole::DistanceKilometers:
                    format = "%.3f km";
                break;

                case FloatRole::LuminousIntensity:
                    factor = 4.0f * math::pi<float>;
                    format = "%.2f lm";
                break;

                case FloatRole::Illuminance:
                    format = "%.2f lm/m²";
                break;

                case FloatRole::None:
                break;
            }

            auto adjuct_factor = [=](float f) {
                if(f > -std::numeric_limits<float>::max() && f < std::numeric_limits<float>::max()) {
                    return f * factor;
                }
                return f;
            };

            float value = f * factor;
            if(ImGui::DragFloat("##drag", &value, 1.0f, adjuct_factor(min), adjuct_factor(max), format)) {
                f = value / factor;
            }
        }

        void inspect(const core::String& name, u32& u, u32 max) override {
            ImGui::PushID(name.data());
            y_defer(ImGui::PopID());

            imgui::table_begin_next_row();
            ImGui::TextUnformatted(name.data());
            ImGui::TableNextColumn();

            if(max >= u32(std::numeric_limits<int>::max())) {
                int value = int(u);
                if(ImGui::DragInt("##drag", &value, 1.0f, 0)) {
                    u = u32(value);
                }
            } else {
                int value = int(u);
                if(ImGui::DragInt("##drag", &value, 1.0f / float(max), 0, int(max))) {
                    u = u32(value);
                }
            }
        }

        void inspect(const core::String& name, bool& b) override {
            ImGui::PushID(name.data());
            y_defer(ImGui::PopID());

            imgui::table_begin_next_row();
            ImGui::TextUnformatted(name.data());
            ImGui::TableNextColumn();
            ImGui::Checkbox("##checkbox", &b);
        }

        void inspect(const core::String& name, ecs::EntityId& id, ecs::ComponentTypeIndex type) override {
            ImGui::PushID(name.data());
            y_defer(ImGui::PopID());

            imgui::table_begin_next_row();
            ImGui::TextUnformatted(name.data());
            ImGui::TableNextColumn();
            id_selector(id, name, _id, _type, type);
        }

        void inspect(const core::String& name, GenericAssetPtr& p) override {
            ImGui::PushID(name.data());
            y_defer(ImGui::PopID());

            imgui::table_begin_next_row();
            ImGui::TextUnformatted(name.data());
            ImGui::TableNextColumn();

            switch(p.type()) {
                case AssetType::Mesh:
                    asset_ptr_selector<StaticMesh>(p, name, _id, _type);
                break;

                case AssetType::Image:
                    if(p.matches<Texture>()) {
                        asset_ptr_selector<Texture>(p, name, _id, _type);
                    } else if(p.matches<IBLProbe>()) {
                        asset_ptr_selector<IBLProbe>(p, name, _id, _type);
                    }
                break;

                case AssetType::Material:
                    asset_ptr_selector<Material>(p, name, _id, _type);
                break;

                default:
                    ImGui::TextDisabled("Unknown asset type");
                break;
            }
        }

    protected:
        bool begin_collection(const core::String& name) override {
            end_table();

            ImGui::Indent();
            if(ImGui::CollapsingHeader(name.data())) {
                if(begin_table()) {
                    return true;
                }
            }

            ImGui::Unindent();
            return false;
        }

        void end_collection() override {
            ImGui::Unindent();
        }

    private:
        bool _in_table = false;
        ecs::EntityId _id;
        EditorComponent* _editor = nullptr;
        EditorWorld* _world = nullptr;
        ecs::ComponentTypeIndex _type = {};
};





Inspector::Inspector() : Widget(ICON_FA_WRENCH " Inspector") {
}

void Inspector::on_gui() {
    EditorWorld& world = current_world();

    const ecs::EntityId selected = current_world().selected_entity();
    const ecs::EntityId id = _locked.is_valid() ? _locked : selected;
    EditorComponent* component = world.component_mut<EditorComponent>(id);

    if(!id.is_valid() || !component) {
        if(const usize selected_count = current_world().selected_entity_count(); selected_count > 1) {
            ImGui::Text("%u selected entities", u32(selected_count));
        } else {
            ImGui::TextUnformatted("No entity selected");
        }
        return;
    }


    {
        core::String name = component->name();
        if(imgui::text_input("Name##name", name)) {
            component->set_name(name);
        }

        ImGui::SameLine();

        bool locked = _locked.is_valid();
        if(ImGui::Checkbox(ICON_FA_LOCK "###lock", &locked)) {
            _locked = locked ? selected : ecs::EntityId();
        }


        ImGui::SameLine();

        ImGui::TextDisabled("(?)");
        if(ImGui::BeginItemTooltip()) {
            ImGui::TextUnformatted(fmt_c_str("ID = {:08x}:{:08x}",id.index(), id.version()));
            ImGui::EndTooltip();
        }
    }

    InspectorPanelInspector inspector(id, component, &world);
    world.inspect_components(id, &inspector);


    ImGui::Separator();

    if(ImGui::Button(ICON_FA_PLUS " Add component")) {
        ImGui::OpenPopup("##addcomponentmenu");
    }

    if(ImGui::BeginPopup("##addcomponentmenu")) {
        for(const auto& [name, info] : EditorWorld::component_types()) {
            const bool enabled = !name.is_empty() && !world.has_component(id, info.type_id) && info.add_or_replace_component;
            if(ImGui::MenuItem(fmt_c_str(ICON_FA_PUZZLE_PIECE " {}", name), nullptr, nullptr, enabled) && enabled) {
                info.add_or_replace_component(world, id);
            }
        }
        ImGui::EndPopup();
    }
}

}

