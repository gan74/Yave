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

#include "Inspector.h"

#include <editor/EditorWorld.h>
#include <editor/UndoStack.h>
#include <editor/widgets/AssetSelector.h>
#include <editor/widgets/EntitySelector.h>
#include <editor/components/EditorComponent.h>
#include <editor/utils/assets.h>
#include <editor/utils/ui.h>
#include <editor/utils/entities.h>

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

#include <type_traits>


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

template<typename T>
class SetterInspector final : public ecs::ComponentInspector {
    public:
        SetterInspector(const core::String& name, ecs::ComponentTypeIndex type, T value) :
                _name(name),
                _type(type),
                _value(value) {
        }

        ~SetterInspector() {
            y_debug_assert(_value_set);
        }

        bool inspect_component_type(ecs::ComponentRuntimeInfo info, bool) override {
            return _is_type = (info.type_id == _type);
        }

        void inspect(const core::String& name, math::Transform<>& tr) override {
            if constexpr(std::is_same_v<T, math::Transform<>>) {
                if(is_property(name)) {
                    tr = _value;
                    _value_set = true;
                }
            }
        }

        void inspect(const core::String& name, math::Vec3& v, Vec3Role) override {
            if constexpr(std::is_same_v<T, math::Vec3>) {
                if(is_property(name)) {
                    v = _value;
                    _value_set = true;
                }
            }
        }

        void inspect(const core::String& name, float& f, FloatRole) override {
            if constexpr(std::is_same_v<T, float>) {
                if(is_property(name)) {
                    f = _value;
                    _value_set = true;
                }
            }
        }

        void inspect(const core::String& name, float& f, float, float, FloatRole) override {
            if constexpr(std::is_same_v<T, float>) {
                if(is_property(name)) {
                    f = _value;
                    _value_set = true;
                }
            }
        }

        void inspect(const core::String& name, u32& u, u32) override {
            if constexpr(std::is_same_v<T, u32>) {
                if(is_property(name)) {
                    u = _value;
                    _value_set = true;
                }
            }
        }

        void inspect(const core::String& name, bool& b) override {
            if constexpr(std::is_same_v<T, bool>) {
                if(is_property(name)) {
                    b = _value;
                    _value_set = true;
                }
            }
        }

        void inspect(const core::String& name, GenericAssetPtr& p) override {
            if constexpr(std::is_same_v<T, GenericAssetPtr>) {
                if(is_property(name)) {
                    p = _value;
                    _value_set = true;
                }
            }
        }

        void inspect(const core::String& name, ecs::EntityId& id, ecs::ComponentTypeIndex) override {
            if constexpr(std::is_same_v<T, ecs::EntityId>) {
                if(is_property(name)) {
                    id = _value;
                    _value_set = true;
                }
            }
        }

    protected:
        bool is_property(const core::String& name) const {
            return _is_type && name == _name;
        }

    private:
        core::String _name;
        ecs::ComponentTypeIndex _type = {};
        T _value;

        bool _value_set = false;
        bool _is_type = false;
};

static void id_selector(ecs::EntityId& property_id, const core::String& name, ecs::EntityId entity_id, ecs::ComponentTypeIndex comp_type, ecs::ComponentTypeIndex target_type) {
    bool browse = false;
    imgui::id_selector(property_id, current_world(), target_type, &browse);
    if(browse) {
        add_child_widget<EntitySelector>(target_type)->set_selected_callback(
            [=](ecs::EntityId new_id) {
                SetterInspector<ecs::EntityId> setter(name, comp_type, new_id);
                current_world().inspect_components(entity_id, &setter);
                return true;
            }
        );
    }
}

template<typename T>
static void asset_ptr_selector(GenericAssetPtr& ptr, const core::String& name, ecs::EntityId id, ecs::ComponentTypeIndex comp_type) {
    y_always_assert(ptr.matches<T>(), "AssetPtr doesn't match given type");

    bool clear = false;
    const AssetType type = ptr.type();
    if(imgui::asset_selector(ptr.id(), type, asset_type_name(type, false, false), &clear)) {
        add_child_widget<AssetSelector>(type)->set_selected_callback(
            [=](AssetId asset) {
                if(const auto loaded = asset_loader().load_res<T>(asset)) {
                    SetterInspector<GenericAssetPtr> setter(name, comp_type, loaded.unwrap());
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
                const math::Vec2 button_pos(ImGui::GetContentRegionAvail().x - button_size, ImGui::GetCursorScreenPos().y);
                ImGui::SetCursorScreenPos(button_pos);
                const bool can_remove = !_world->is_component_required(_id, info.type_id);
                const bool remove_component = ImGui::InvisibleButton(ICON_FA_TRASH "###invisible", math::Vec2(button_size));

                ImGui::SameLine();
                ImGui::SetCursorPosX(0.0);
                open = ImGui::CollapsingHeader(fmt_c_str(ICON_FA_PUZZLE_PIECE " {}", info.clean_component_name()), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow);

                ImGui::SameLine();
                ImGui::SetCursorScreenPos(button_pos);
                ImGui::BeginDisabled(!can_remove);
                ImGui::Button(ICON_FA_TRASH, math::Vec2(button_size));
                ImGui::EndDisabled();

                if(remove_component) {
                    if(!can_remove) {
                        log_msg(fmt("{} can not be deleted as it is required by some other component", info.clean_component_name()), Log::Warning);
                    } else {
                        undo_enabled_remove_component(_id, info);
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

            auto [pos, rot, scale] = tr.decompose();
            bool changed = false;

            // position
            {
                begin_property_row("Position");
                changed |= imgui::position_input("##position", pos);
            }


            // rotation
            {
                begin_property_row("Rotation");

                auto is_same_angle = [&](math::Vec3 a, math::Vec3 b) {
                    const auto qa = math::Quaternion<>::from_euler(a);
                    const auto qb = math::Quaternion<>::from_euler(b);
                    for(usize i = 0; i != 3; ++i) {
                        math::Vec3 v;
                        v[i] = 1.0f;
                        if((qa(v) - qb(v)).sq_length() > 0.001f) {
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
                    changed = true;
                }
            }

            // scale
            {
                begin_property_row("Scale");

                float scalar_scale = scale.dot(math::Vec3(1.0f / 3.0f));
                if(ImGui::DragFloat("##scale", &scalar_scale, 0.1f, 0.0f, 0.0f, "%.3f")) {
                    scale = std::max(0.001f, scalar_scale);
                    changed = true;
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

            if(changed) {
                tr = math::Transform<>(pos, rot, scale);
            }
        }

        void inspect(const core::String& name, math::Vec3& v, Vec3Role role) override {
            ImGui::PushID(name.data());
            y_defer(ImGui::PopID());

            begin_property_row(name);

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

            begin_property_row(name);

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

            begin_property_row(name);

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

            begin_property_row(name);
            ImGui::Checkbox("##checkbox", &b);
        }

        void inspect(const core::String& name, ecs::EntityId& id, ecs::ComponentTypeIndex type) override {
            ImGui::PushID(name.data());
            y_defer(ImGui::PopID());

            begin_property_row(name);
            id_selector(id, name, _id, _type, type);
        }

        void inspect(const core::String& name, GenericAssetPtr& p) override {
            ImGui::PushID(name.data());
            y_defer(ImGui::PopID());

            begin_property_row(name);

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

        void begin_property_row(std::string_view name) {
            imgui::table_begin_next_row();
            ImGui::TextUnformatted(name.data(), name.data() + name.size());
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().FramePadding.x * 2.0f);
        }


        ecs::EntityId _id;
        EditorComponent* _editor = nullptr;
        EditorWorld* _world = nullptr;
        ecs::ComponentTypeIndex _type = {};


        bool _in_table = false;
};


class UndoRedoInspector final : public InspectorPanelInspector {
    public:
        template<typename... Args>
        UndoRedoInspector(Args&&... args) : InspectorPanelInspector(y_fwd(args)...) {
        }

        void inspect(const core::String& name, math::Transform<>& tr) override {
            const auto _ = check_changed(name, tr);
            InspectorPanelInspector::inspect(name, tr);
        }

        void inspect(const core::String& name, math::Vec3& v, ecs::ComponentInspector::Vec3Role role) override {
            const auto _ = check_changed(name, v);
            InspectorPanelInspector::inspect(name, v, role);
        }

        void inspect(const core::String& name, float& f, float min, float max, ecs::ComponentInspector::FloatRole role) override {
            const auto _ = check_changed(name, f);
            InspectorPanelInspector::inspect(name, f, min, max, role);
        }

        void inspect(const core::String& name, u32& u, u32 max) override {
            const auto _ = check_changed(name, u);
            InspectorPanelInspector::inspect(name, u, max);
        }

        void inspect(const core::String& name, bool& b) override {
            const auto _ = check_changed(name, b);
            InspectorPanelInspector::inspect(name, b);
        }

        void inspect(const core::String& name, ecs::EntityId& id, ecs::ComponentTypeIndex type) override {
            const auto _ = check_changed(name, id);
            InspectorPanelInspector::inspect(name, id, type);
        }

        void inspect(const core::String& name, GenericAssetPtr& p) override {
            const auto _ = check_changed(name, p);
            InspectorPanelInspector::inspect(name, p);
        }

    private:
        template<typename T>
        auto check_changed(const core::String& name, T& t) {
            return ScopeGuard([this, name, ptr = &t, value = t] {
                static_assert(!std::is_reference_v<decltype(value)>);
                const T& new_value = *ptr;
                if(new_value != value) {
                    static const auto undo_id = UndoStack::generate_static_id();
                    undo_stack().push(
                        fmt_to_owned("{}.{} changed", _world->component_type_name(_type), name),
                        [name, value, id = _id, type = _type](EditorWorld& world) {
                            SetterInspector<T> setter(name, type, value);
                            world.inspect_components(id, &setter);
                        },
                        [name, new_value, id = _id, type = _type](EditorWorld& world) {
                            SetterInspector<T> setter(name, type, new_value);
                            world.inspect_components(id, &setter);
                        },
                        undo_id
                    );
                }
            });
        }
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
        ImGui::BeginGroup();

        core::String name = component->name();
        if(imgui::text_input("Name##name", name)) {
            undo_enabled_rename(id, name);
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

        ImGui::EndGroup();
    }

    if(component->is_prefab()) {
        ImGui::BeginGroup();
        imgui::text_read_only("Prefab", fmt("ID = {:08x}:{:08x}",id.index(), id.version()));
        ImGui::EndGroup();
    }


    UndoRedoInspector inspector(id, component, &world);
    world.inspect_components(id, &inspector);


    ImGui::Separator();

    if(ImGui::Button(ICON_FA_PLUS " Add component")) {
        ImGui::OpenPopup("##addcomponentmenu");
    }

    if(ImGui::BeginPopup("##addcomponentmenu")) {
        for(const auto& [name, info] : EditorWorld::component_types()) {
            const bool enabled = !name.is_empty() && !world.has_component(id, info.type_id) && info.add_or_replace_component;
            if(ImGui::MenuItem(fmt_c_str(ICON_FA_PUZZLE_PIECE " {}", name), nullptr, nullptr, enabled) && enabled) {
                undo_enabled_add_component(id, info);
            }
        }
        ImGui::EndPopup();
    }
}

}

