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
#include "EntityView.h"

#include <editor/Settings.h>
#include <editor/Selection.h>
#include <editor/EditorWorld.h>
#include <editor/utils/ui.h>
#include <editor/widgets/AssetSelector.h>
#include <editor/components/EditorComponent.h>

#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/components/DirectionalLightComponent.h>

#include <yave/graphics/device/DeviceResources.h>
#include <yave/utils/FileSystemModel.h>
#include <yave/assets/AssetLoader.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <external/imgui/yave_imgui.h>

namespace editor {

static void add_debug_entities() {
    EditorWorld& world = current_world();

    const auto mesh = device_resources()[DeviceResources::CubeMesh];
    const auto material = device_resources()[DeviceResources::EmptyMaterial];

    const usize entity_count = app_settings().debug.entity_count;
    const usize side = usize(std::max(1.0, std::cbrt(entity_count)));
    for(usize i = 0; i != entity_count; ++i) {
        const ecs::EntityId entity = world.create_entity<StaticMeshComponent>();
        world.set_entity_name(entity, "Debug entity");

        const math::Vec3 pos = math::Vec3(i / (side * side), (i / side) % side, i % side) - (side * 0.5f);
        world.component<TransformableComponent>(entity)->position() = pos * 10.0f;
        *world.component<StaticMeshComponent>(entity) = StaticMeshComponent(mesh, material);
        world.component<EditorComponent>(entity)->set_hidden_in_editor(true);
    }
}

editor_action("Add debug entities", add_debug_entities)



EntityView::EntityView() : Widget(ICON_FA_CUBES " Entities") {
}

void EntityView::paint_view() {
    EditorWorld& world = current_world();

    const bool display_hidden = app_settings().debug.display_hidden_entities;

    if(ImGui::BeginChild("##entities", ImVec2(), true)) {
        imgui::alternating_rows_background();
        for(ecs::EntityId id : world.ids()) {
            const EditorComponent* comp = world.component<EditorComponent>(id);
            if(!comp) {
                log_msg(fmt("Entity with id % is missing EditorComponent", id.index()), Log::Warning);
                continue;
            }

            if(!display_hidden && comp->is_hidden_in_editor()) {
                continue;
            }

            const bool selected = selection().selected_entity() == id;
            if(ImGui::Selectable(fmt_c_str("% %##%", world.entity_icon(id), comp->name(), id.index()), selected)) {
                 selection().set_selected(id);
            }
            if(ImGui::IsItemHovered()) {
                _hovered = id;
            }
        }
    }
    ImGui::EndChild();
}

void EntityView::on_gui() {
    EditorWorld& world = current_world();

    if(ImGui::Button(ICON_FA_PLUS, math::Vec2(24))) {
        ImGui::OpenPopup("Add entity");
    }


    if(ImGui::BeginPopup("Add entity")) {
        ecs::EntityId ent;
        if(ImGui::MenuItem(ICON_FA_PLUS " Add empty entity")) {
            ent = world.create_entity();
        }
        ImGui::Separator();

        if(ImGui::MenuItem("Add prefab")) {
            add_detached_widget<AssetSelector>(AssetType::Prefab)->set_selected_callback(
                [](AssetId asset) {
                    if(const auto prefab = asset_loader().load_res<ecs::EntityPrefab>(asset)) {
                        const ecs::EntityId id = current_world().create_entity(*prefab.unwrap());
                        selection().set_selected(id);

                        if(const auto name = asset_store().name(asset)) {
                            current_world().set_entity_name(id, fmt("% (Prefab)", asset_store().filesystem()->filename(name.unwrap())));
                        }
                    }
                    return true;
                });
        }


        ImGui::Separator();

        if(ImGui::MenuItem(ICON_FA_LIGHTBULB " Add Point light")) {
            ent = world.create_named_entity("Point light", ecs::StaticArchetype<PointLightComponent>());
        }

        if(ImGui::MenuItem(ICON_FA_VIDEO " Add Spot light")) {
            ent = world.create_named_entity("Spot light", ecs::StaticArchetype<SpotLightComponent>());
        }

        if(ent.is_valid()) {
            selection().set_selected(ent);
        }

        y_debug_assert(!ent.is_valid() || world.has<EditorComponent>(ent));
        y_debug_assert(world.required_components().size() > 0);

        ImGui::EndPopup();
    }



    ImGui::Spacing();
    paint_view();



    if(_hovered.is_valid()) {
        if(ImGui::IsMouseReleased(1)) {
            ImGui::OpenPopup("##contextmenu");
        }

        if(ImGui::BeginPopup("##contextmenu")) {
            if(ImGui::BeginMenu(ICON_FA_PLUS " Add component")) {

                for(const auto& [name, info] : EditorWorld::component_types()) {
                    const bool enabled = !name.is_empty() && !world.has(_hovered, info.type_id) && info.add_component;
                    if(ImGui::MenuItem(fmt_c_str(ICON_FA_PUZZLE_PIECE " %", name), nullptr, nullptr, enabled) && enabled) {
                        info.add_component(world, _hovered);
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::Separator();
            if(ImGui::Selectable(ICON_FA_TRASH " Delete")) {
                world.remove_entity(_hovered);
                // we don't unselect the ID to make sure that we can handle case where the id is invalid
            }

            if(ImGui::Selectable("Duplicate")) {
                const ecs::EntityPrefab prefab = world.create_prefab(_hovered);
                const ecs::EntityId copy = world.create_named_entity(core::String(world.entity_name(_hovered)) + " (Copy)", prefab);
                selection().set_selected(copy);
            }

            ImGui::EndPopup();
        } else {
            _hovered = ecs::EntityId();
        }
    }
}

}

