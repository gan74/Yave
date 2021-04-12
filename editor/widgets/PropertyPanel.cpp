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

#include "PropertyPanel.h"
#include "AssetSelector.h"

#include <editor/Selection.h>
#include <editor/EditorWorld.h>
#include <editor/components/EditorComponent.h>
#include <editor/utils/ui.h>

#include <yave/components/PointLightComponent.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/components/DirectionalLightComponent.h>
#include <yave/components/SkyLightComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/components/TransformableComponent.h>
#include <yave/components/AtmosphereComponent.h>

#include <yave/utils/color.h>
#include <yave/assets/AssetLoader.h>
#include <yave/graphics/images/ImageData.h>
#include <yave/material/Material.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/meshes/MeshData.h>

#include <external/imgui/yave_imgui.h>

#include <y/utils/log.h>
#include <y/utils/format.h>


#define EDITOR_WIDGET_REG_RUNNER y_create_name_with_prefix(runner)
#define EDITOR_WIDGET_FUNC y_create_name_with_prefix(widget)

#define editor_widget_draw_func(...)                                                                        \
    static void EDITOR_WIDGET_FUNC(ecs::EntityId id);                                                       \
    namespace {                                                                                             \
        class EDITOR_WIDGET_REG_RUNNER {                                                                    \
            EDITOR_WIDGET_REG_RUNNER() {                                                                    \
                comp_data.func = &EDITOR_WIDGET_FUNC;                                                       \
                detail::register_component_widget(&comp_data);                                              \
            }                                                                                               \
            static detail::ComponentWidgetData comp_data;                                                   \
            static EDITOR_WIDGET_REG_RUNNER runner;                                                         \
        };                                                                                                  \
        detail::ComponentWidgetData EDITOR_WIDGET_REG_RUNNER::comp_data = {};                               \
        EDITOR_WIDGET_REG_RUNNER EDITOR_WIDGET_REG_RUNNER::runner = {};                                     \
    }                                                                                                       \
    void EDITOR_WIDGET_FUNC(__VA_ARGS__)


namespace editor {

void draw_component_widgets(ecs::EntityId id);

PropertyPanel::PropertyPanel() : Widget(ICON_FA_WRENCH " Properties") {
}

void PropertyPanel::on_gui() {
    if(!selection().has_selected_entity()) {
        return;
    }

    ecs::EntityId id = selection().selected_entity();
    if(id.is_valid()) {
        ImGui::PushID("widgets");
        draw_component_widgets(id);
        ImGui::PopID();
    }
}


namespace detail {
using component_widget_func_t = void (*)(ecs::EntityId id);
struct ComponentWidgetData {
    component_widget_func_t func = nullptr;
    ComponentWidgetData* next = nullptr;
};

static ComponentWidgetData* first_component = nullptr;

void register_component_widget(ComponentWidgetData* data) {
    data->next = first_component;
    first_component = data;
}

}

void draw_component_widgets(ecs::EntityId id) {
    usize index = 0;
    for(detail::ComponentWidgetData* data = detail::first_component; data; data = data->next) {
        y_debug_assert(data->func);

        ImGui::PushID(fmt_c_str("wid %", index++));
        data->func(id);
        ImGui::PopID();
    }
}


/**************************************************************************
*                               Lights
**************************************************************************/

template<typename T>
static void light_widget(T* light) {
    const int color_flags = ImGuiColorEditFlags_NoSidePreview |
                      // ImGuiColorEditFlags_NoSmallPreview |
                      ImGuiColorEditFlags_NoAlpha |
                      ImGuiColorEditFlags_Float |
                      ImGuiColorEditFlags_InputRGB;

    const math::Vec4 color(light->color(), 1.0f);


    ImGui::Text("Light color");
    ImGui::NextColumn();
    if(ImGui::ColorButton("Color", color, color_flags)) {
        ImGui::OpenPopup("Color");
    }

    if(ImGui::BeginPopup("Color")) {
        ImGui::ColorPicker3("##lightpicker", light->color().begin(), color_flags);

        float kelvin = std::clamp(rgb_to_k(light->color()), 1000.0f, 12000.0f);
        if(ImGui::SliderFloat("##temperature", &kelvin, 1000.0f, 12000.0f, "%.0f°K")) {
            light->color() = k_to_rbg(kelvin);
        }

        ImGui::EndPopup();
    }


    ImGui::NextColumn();
    ImGui::Text("Intensity");
    ImGui::NextColumn();
    ImGui::DragFloat("##intensity", &light->intensity(), 1.0f, 0.0f, std::numeric_limits<float>::max(), "%.2f");
}

editor_widget_draw_func(ecs::EntityId id) {
    PointLightComponent* light = current_world().component<PointLightComponent>(id);
    if(!light) {
        return;
    }

    if(!ImGui::CollapsingHeader("Point light", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }


    ImGui::Columns(2);
    {
        light_widget(light);

        ImGui::NextColumn();
        ImGui::Text("Radius");
        ImGui::NextColumn();
        ImGui::DragFloat("##radius", &light->radius(), 1.0f, 0.0f, std::numeric_limits<float>::max(), "%.2f");

        ImGui::NextColumn();
        ImGui::Text("Falloff");
        ImGui::NextColumn();
        ImGui::DragFloat("##falloff", &light->falloff(), 0.1f, 0.0f, std::numeric_limits<float>::max(), "%.2f");
    }
    ImGui::Columns(1);
}

editor_widget_draw_func(ecs::EntityId id) {
    SpotLightComponent* light = current_world().component<SpotLightComponent>(id);
    if(!light) {
        return;
    }

    if(!ImGui::CollapsingHeader("Spot light", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }


    ImGui::Columns(2);
    {
        light_widget(light);

        ImGui::NextColumn();
        ImGui::Text("Radius");
        ImGui::NextColumn();
        ImGui::DragFloat("##radius", &light->radius(), 1.0f, 0.0f, std::numeric_limits<float>::max(), "%.2f");

        ImGui::NextColumn();
        ImGui::Text("Falloff");
        ImGui::NextColumn();
        ImGui::DragFloat("##falloff", &light->falloff(), 0.1f, 0.0f, std::numeric_limits<float>::max(), "%.2f");

        ImGui::NextColumn();
        ImGui::Text("Angle");
        ImGui::NextColumn();
        float angle = math::to_deg(light->half_angle() * 2.0f);
        if(ImGui::DragFloat("##angle", &angle, 0.1f, 0.0f, 360.0f, "%.2f°")) {
            light->half_angle() = math::to_rad(angle * 0.5f);
        }

        ImGui::NextColumn();
        ImGui::Text("Exponent");
        ImGui::NextColumn();
        ImGui::DragFloat("##exponent", &light->angle_exponent(), 0.1f, 0.0f, 10.0f, "%.2f");


        ImGui::NextColumn();
        ImGui::Text("Cast shadows");
        ImGui::NextColumn();
        ImGui::Checkbox("##shadows", &light->cast_shadow());


        ImGui::NextColumn();
        ImGui::Text("Shadow LoD");
        ImGui::NextColumn();

        int lod = light->shadow_lod();
        if(ImGui::DragInt("LoD", &lod, 1.0f / 8.0f, 0, 8)) {
            light->shadow_lod() = lod;
        }

        /*if(light->cast_shadow()) {
            ImGui::NextColumn();
            ImGui::Text("Shadow bias");
            ImGui::NextColumn();
            ImGui::InputFloat("C##bias", &light->depth_bias().x());
            ImGui::InputFloat("S##bias", &light->depth_bias().y());
        }*/
    }
    ImGui::Columns(1);
}

editor_widget_draw_func(ecs::EntityId id) {
    DirectionalLightComponent* light = current_world().component<DirectionalLightComponent>(id);
    if(!light) {
        return;
    }

    if(!ImGui::CollapsingHeader("Directional light", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }


    ImGui::Columns(2);
    {
        light_widget(light);

        ImGui::NextColumn();
        ImGui::Text("Direction");
        ImGui::NextColumn();
        //ImGui::InputFloat3("##direction", light->direction().data(), "%.2f");

        const math::Vec3 dir = light->direction().normalized();
        float elevation = -math::to_deg(std::asin(dir.z()));
        const math::Vec2 dir_2d = dir.to<2>().normalized();
        float azimuth = math::to_deg(std::copysign(std::acos(dir_2d.x()), std::asin(dir_2d.y())));

        bool changed = false;

        changed |= ImGui::DragFloat("Azimuth", &azimuth, 1.0, -180.0f, 180.0f, "%.2f°");
        changed |= ImGui::DragFloat("Elevation", &elevation, 1.0, -90.0f, 90.0f, "%.2f°");

        if(changed) {
            elevation = -math::to_rad(elevation);
            azimuth = math::to_rad(azimuth);

            math::Vec3 new_dir;
            new_dir.z() = std::sin(elevation);
            new_dir.to<2>() = math::Vec2(std::cos(azimuth), std::sin(azimuth)) * std::cos(elevation);

            light->direction() = new_dir;
        }
    }
    ImGui::Columns(1);
}


editor_widget_draw_func(ecs::EntityId id) {
    SkyLightComponent* sky = current_world().component<SkyLightComponent>(id);
    if(!sky) {
        return;
    }

    if(!ImGui::CollapsingHeader("Sky", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    ImGui::Columns(2);
    {
        ImGui::Text("Envmap");
        ImGui::NextColumn();
        bool clear = false;
        if(imgui::asset_selector(sky->probe().id(), AssetType::Image, "Envmap", &clear)) {
            add_child_widget<AssetSelector>(AssetType::Image)->set_selected_callback(
                [=](AssetId asset) {
                    if(const auto probe = asset_loader().load_res<IBLProbe>(asset)) {
                        if(SkyLightComponent* sky = current_world().component<SkyLightComponent>(id)) {
                            sky->probe() = probe.unwrap();
                        }
                    }
                    return true;
                });
        } else if(clear) {
            sky->probe() = AssetPtr<IBLProbe>();
        }
    }
    ImGui::Columns(1);
}


/**************************************************************************
*                               Meshes
**************************************************************************/

editor_widget_draw_func(ecs::EntityId id) {
    StaticMeshComponent* static_mesh = current_world().component<StaticMeshComponent>(id);
    if(!static_mesh) {
        return;
    }
    if(!ImGui::CollapsingHeader("Static mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    ImGui::Columns(2);
    {

        for(usize i = 0; i != static_mesh->sub_meshes().size(); ++i) {
            StaticMeshComponent::SubMesh& sub_mesh = static_mesh->sub_meshes()[i];

            auto find_sub_mesh = [id, i, sub = sub_mesh]() -> StaticMeshComponent::SubMesh* {
                static_assert(!std::is_reference_v<decltype(sub)>);
                if(StaticMeshComponent* comp = current_world().component<StaticMeshComponent>(id)) {
                    if(comp->sub_meshes().size() > i && comp->sub_meshes()[i] == sub) {
                        return &comp->sub_meshes()[i];
                    }
                }
                return nullptr;
            };

            {
                ImGui::Text("Material");
                ImGui::NextColumn();

                bool clear = false;
                if(imgui::asset_selector(sub_mesh.material.id(), AssetType::Material, "Material", &clear)) {
                    add_child_widget<AssetSelector>(AssetType::Material)->set_selected_callback(
                        [=](AssetId asset) {
                            if(const auto material = asset_loader().load_res<Material>(asset)) {
                                if(StaticMeshComponent::SubMesh* sub = find_sub_mesh()) {
                                    sub->material = material.unwrap();
                                }
                            }
                            return true;
                        });
                } else if(clear) {
                    sub_mesh.material = AssetPtr<Material>();
                }
            }

            {
                ImGui::NextColumn();
                ImGui::Text("Mesh");
                ImGui::NextColumn();

                bool clear = false;
                if(imgui::asset_selector(sub_mesh.mesh.id(), AssetType::Mesh, "Mesh", &clear)) {
                    add_child_widget<AssetSelector>(AssetType::Mesh)->set_selected_callback(
                        [=](AssetId asset) {
                            if(const auto mesh = asset_loader().load_res<StaticMesh>(asset)) {
                                if(StaticMeshComponent::SubMesh* sub = find_sub_mesh()) {
                                    sub->mesh = mesh.unwrap();
                                }
                            }
                            return true;
                        });
                } else if(clear) {
                    sub_mesh.mesh = AssetPtr<StaticMesh>();
                }
            }

            ImGui::NextColumn();
            ImGui::Separator();
        }

        ImGui::Columns(1);

        if(ImGui::Button("Add sub mesh")) {
            static_mesh->sub_meshes().emplace_back();
        }
    }
}



/**************************************************************************
*                               Transformable
**************************************************************************/

editor_widget_draw_func(ecs::EntityId id) {
    TransformableComponent* component = current_world().component<TransformableComponent>(id);
    if(!component) {
        return;
    }

    if(!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    ImGui::Columns(2);
    {
        auto [pos, rot, scale] = component->transform().decompose();

        // position
        {
            ImGui::Text("Position");
            ImGui::NextColumn();
            ImGui::InputFloat("X", &pos.x(), 0.0f, 0.0f, "%.2f");
            ImGui::InputFloat("Y", &pos.y(), 0.0f, 0.0f, "%.2f");
            ImGui::InputFloat("Z", &pos.z(), 0.0f, 0.0f, "%.2f");
        }

        ImGui::Separator();

        // rotation
        {
            ImGui::NextColumn();
            ImGui::Text("Rotation");
            ImGui::NextColumn();

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


            math::Vec3 actual_euler = rot.to_euler();
            auto& euler = current_world().component<EditorComponent>(id)->euler();

            if(!is_same_angle(actual_euler, euler)) {
                euler = actual_euler;
            }

            const std::array indexes = {math::Quaternion<>::YawIndex, math::Quaternion<>::PitchIndex, math::Quaternion<>::RollIndex};
            const std::array names = {"Yaw", "Pitch", "Roll"};

            for(usize i = 0; i != 3; ++i) {
                float angle = math::to_deg(euler[indexes[i]]);
                if(ImGui::DragFloat(names[i], &angle, 1.0, -180.0f, 180.0f, "%.2f")) {
                    euler[indexes[i]] = math::to_rad(angle);
                    rot = math::Quaternion<>::from_euler(euler);
                }
            }
        }

        component->set_transform(math::Transform<>(pos, rot, scale));
    }
    ImGui::Columns(1);
}



/**************************************************************************
*                               Atmosphere
**************************************************************************/

editor_widget_draw_func(ecs::EntityId id) {
    AtmosphereComponent* component = current_world().component<AtmosphereComponent>(id);
    if(!component) {
        return;
    }

    if(!ImGui::CollapsingHeader("Atmosphere", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    ImGui::InputFloat("Planet radius", &component->planet_radius, 0.0f, 0.0f, "%.6f km");
    ImGui::InputFloat("Atmosphere height", &component->atmosphere_height, 0.0f, 0.0f, "%.6f km");
    ImGui::InputFloat("Density falloff", &component->density_falloff, 0.0f, 0.0f, "%.2f");
    ImGui::InputFloat("Scaterring strength", &component->scattering_strength, 0.0f, 0.0f, "%.2f");
}




/**************************************************************************
*                               Editor
**************************************************************************/

editor_widget_draw_func(ecs::EntityId id) {
    EditorComponent* component = current_world().component<EditorComponent>(id);
    if(!component) {
        return;
    }

    if(!ImGui::CollapsingHeader("Entity")) {
        return;
    }

    const core::String& name = component->name();

    std::array<char, 1024> name_buffer;
    std::fill(name_buffer.begin(), name_buffer.end(), 0);
    std::copy(name.begin(), name.end(), name_buffer.begin());

    if(ImGui::InputText("Name", name_buffer.data(), name_buffer.size())) {
        component->set_name(name_buffer.data());
    }

    ImGui::Text("Id: %x", id.index());
}


}
