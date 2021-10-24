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

#include "ComponentPanelWidgets.h"
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

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <external/imgui/yave_imgui.h>

#include <cinttypes>

namespace editor {

ComponentPanelWidgetBase::Link* ComponentPanelWidgetBase::_first_link = nullptr;

void ComponentPanelWidgetBase::register_link(Link* link) {
    link->next = _first_link;
    _first_link = link;
}



template<typename CRTP, typename T>
struct LightComponentWidget : public ComponentPanelWidget<CRTP, T> {
    static void basic_properties(T* light) {
        const int color_flags = ImGuiColorEditFlags_NoSidePreview |
                          // ImGuiColorEditFlags_NoSmallPreview |
                          ImGuiColorEditFlags_NoAlpha |
                          ImGuiColorEditFlags_Float |
                          ImGuiColorEditFlags_InputRGB;

        const math::Vec4 color(light->color(), 1.0f);

        {
            ImGui::TextUnformatted("Light color");
            ImGui::TableNextColumn();

            if(ImGui::ColorButton("Color", color, color_flags)) {
                ImGui::OpenPopup("##color");
            }

            if(ImGui::BeginPopup("##color")) {
                ImGui::ColorPicker3("##lightpicker", light->color().begin(), color_flags);

                float kelvin = std::clamp(rgb_to_k(light->color()), 1000.0f, 12000.0f);
                if(ImGui::SliderFloat("##temperature", &kelvin, 1000.0f, 12000.0f, "%.0f°K")) {
                    light->color() = k_to_rbg(kelvin);
                }

                ImGui::EndPopup();
            }
        }

        imgui::table_begin_next_row();

        {
            ImGui::TextUnformatted("Intensity");
            ImGui::TableNextColumn();
            ImGui::DragFloat("##intensity", &light->intensity(), 1.0f, 0.0f, std::numeric_limits<float>::max(), "%.2f");
        }
    }

    static void shadow_properties(T* light) {

        {
            ImGui::TextUnformatted("Cast shadows");
            ImGui::TableNextColumn();
            ImGui::Checkbox("##shadows", &light->cast_shadow());
        }

        imgui::table_begin_next_row();

        {
            ImGui::TextUnformatted("Shadow LoD");
            ImGui::TableNextColumn();

            int lod = light->shadow_lod();
            if(ImGui::DragInt("LoD", &lod, 1.0f / 8.0f, 0, 8)) {
                light->shadow_lod() = lod;
            }
        }
    }
};

struct PointLightComponentWidget : public LightComponentWidget<PointLightComponentWidget, PointLightComponent> {
    void on_gui(ecs::EntityId, PointLightComponent* light) {
        basic_properties(light);

        imgui::table_begin_next_row();

        {
            ImGui::TextUnformatted("Radius");
            ImGui::TableNextColumn();
            ImGui::DragFloat("##radius", &light->radius(), 1.0f, 0.0f, std::numeric_limits<float>::max(), "%.2f");
        }

        imgui::table_begin_next_row();

        {
            ImGui::TextUnformatted("Falloff");
            ImGui::TableNextColumn();
            ImGui::DragFloat("##falloff", &light->falloff(), 0.1f, 0.0f, std::numeric_limits<float>::max(), "%.2f");
        }
    }
};

struct DirectionalLightComponentWidget : public LightComponentWidget<DirectionalLightComponentWidget, DirectionalLightComponent> {
    void on_gui(ecs::EntityId, DirectionalLightComponent* light) {
        basic_properties(light);

        imgui::table_begin_next_row();

        {
            ImGui::TextUnformatted("Direction");
            ImGui::TableNextColumn();

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

        imgui::table_begin_next_row();

        shadow_properties(light);

        imgui::table_begin_next_row();

        {
            ImGui::TextUnformatted("Cascade distance");
            ImGui::TableNextColumn();
            ImGui::DragFloat("##dist", &light->cascade_distance(), 1.0f, 10.0f, std::numeric_limits<float>::max(), "%.2f");
        }

    }
};

struct SpotLightComponentWidget : public LightComponentWidget<SpotLightComponentWidget, SpotLightComponent> {
    void on_gui(ecs::EntityId, SpotLightComponent* light) {
        basic_properties(light);

        imgui::table_begin_next_row();

        {
            ImGui::TextUnformatted("Radius");
            ImGui::TableNextColumn();
            ImGui::DragFloat("##radius", &light->radius(), 1.0f, 0.0f, std::numeric_limits<float>::max(), "%.2f");
        }

        imgui::table_begin_next_row();

        {
            ImGui::TextUnformatted("Falloff");
            ImGui::TableNextColumn();
            ImGui::DragFloat("##falloff", &light->falloff(), 0.1f, 0.0f, std::numeric_limits<float>::max(), "%.2f");
        }

        imgui::table_begin_next_row();

        {
            ImGui::TextUnformatted("Angle");
            ImGui::TableNextColumn();
            float angle = math::to_deg(light->half_angle() * 2.0f);
            if(ImGui::DragFloat("##angle", &angle, 0.1f, 0.0f, 360.0f, "%.2f°")) {
                light->half_angle() = math::to_rad(angle * 0.5f);
            }
        }

        imgui::table_begin_next_row();

        {
            ImGui::TextUnformatted("Exponent");
            ImGui::TableNextColumn();
            ImGui::DragFloat("##exponent", &light->angle_exponent(), 0.1f, 0.0f, 10.0f, "%.2f");
        }

        imgui::table_begin_next_row();

        shadow_properties(light);
    }
};

struct SkyLightComponentWidget : public ComponentPanelWidget<SkyLightComponentWidget, SkyLightComponent> {
    void on_gui(ecs::EntityId id, SkyLightComponent* sky) {

        ImGui::TextUnformatted("Envmap");
        ImGui::TableNextColumn();
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
};

struct StaticMeshComponentWidget : public ComponentPanelWidget<StaticMeshComponentWidget, StaticMeshComponent> {
    void on_gui(ecs::EntityId id, StaticMeshComponent* static_mesh) {
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

            imgui::table_begin_next_row();

            {
                ImGui::TextUnformatted("Material");
                ImGui::TableNextColumn();

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

            imgui::table_begin_next_row();

            {
                ImGui::TextUnformatted("Mesh");
                ImGui::TableNextColumn();

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
        }

        imgui::table_begin_next_row(1);

        if(ImGui::Button("Add sub mesh")) {
            static_mesh->sub_meshes().emplace_back();
        }
    }
};

struct AtmosphereComponentWidget : public ComponentPanelWidget<AtmosphereComponentWidget, AtmosphereComponent> {
    void on_gui(ecs::EntityId , AtmosphereComponent* component) {

        ImGui::TextUnformatted("Planet radius");
        ImGui::TableNextColumn();
        ImGui::InputFloat("##radius", &component->planet_radius, 0.0f, 0.0f, "%.6f km");

        imgui::table_begin_next_row();

        ImGui::TextUnformatted("Atmosphere height");
        ImGui::TableNextColumn();
        ImGui::InputFloat("##height", &component->planet_radius, 0.0f, 0.0f, "%.6f km");

        imgui::table_begin_next_row();

        ImGui::TextUnformatted("Density falloff");
        ImGui::TableNextColumn();
        ImGui::InputFloat("##density", &component->planet_radius, 0.0f, 0.0f, "%.6f km");

        imgui::table_begin_next_row();

        ImGui::TextUnformatted("Scaterring strength");
        ImGui::TableNextColumn();
        ImGui::InputFloat("##strength", &component->planet_radius, 0.0f, 0.0f, "%.6f km");
    }
};

struct TransformableComponentWidget : public ComponentPanelWidget<TransformableComponentWidget, TransformableComponent> {
    void on_gui(ecs::EntityId id, TransformableComponent* component) {
        auto [pos, rot, scale] = component->transform().decompose();

        // position
        {
            ImGui::TextUnformatted("Position");
            ImGui::TableNextColumn();

            ImGui::InputFloat("##x", &pos.x(), 0.0f, 0.0f, "%.2f");
            ImGui::InputFloat("##y", &pos.y(), 0.0f, 0.0f, "%.2f");
            ImGui::InputFloat("##z", &pos.z(), 0.0f, 0.0f, "%.2f");
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
};

struct EditorComponentWidget : public ComponentPanelWidget<EditorComponentWidget, EditorComponent> {
    void on_gui(ecs::EntityId id, EditorComponent* component) {
        const core::String& name = component->name();

        {
            ImGui::TextUnformatted("Name");
            ImGui::TableNextColumn();

            std::array<char, 1024> buffer = {};
            std::copy(name.begin(), name.end(), buffer.begin());

            if(ImGui::InputText("##name", buffer.data(), buffer.size())) {
                component->set_name(buffer.data());
            }
        }

        imgui::table_begin_next_row();

        {
            ImGui::TextUnformatted("Id");
            ImGui::TableNextColumn();

            std::array<char, 16> buffer = {};
            std::snprintf(buffer.data(), buffer.size(), "%08" PRIu32, id.index());

            ImGui::InputText("##id", buffer.data(), buffer.size(), ImGuiInputTextFlags_ReadOnly);
        }
    }
};

}
