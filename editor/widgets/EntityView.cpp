/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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
#include <editor/widgets/Renamer.h>
#include <editor/widgets/AssetSelector.h>
#include <editor/widgets/DeletionDialog.h>
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
    y_profile();

    EditorWorld& world = current_world();

    const auto mesh = device_resources()[DeviceResources::CubeMesh];
    const auto material = device_resources()[DeviceResources::EmptyMaterial];

    const float spacing =  app_settings().debug.entity_spacing;
    const usize entity_count = app_settings().debug.entity_count;
    const usize side = usize(std::max(1.0, std::cbrt(entity_count)));
    for(usize i = 0; i != entity_count; ++i) {
        const ecs::EntityId entity = world.create_entity<StaticMeshComponent>();
        world.set_entity_name(entity, "Debug entity");

        const math::Vec3 pos = math::Vec3(i / (side * side), (i / side) % side, i % side) - (side * 0.5f);
        world.component<TransformableComponent>(entity)->set_position(pos * spacing);
        *world.component<StaticMeshComponent>(entity) = StaticMeshComponent(mesh, material);
        world.component<EditorComponent>(entity)->set_hidden_in_editor(true);
    }
}

static void add_prefab() {
    add_detached_widget<AssetSelector>(AssetType::Prefab, "Add prefab")->set_selected_callback(
        [](AssetId asset) {
            const ecs::EntityId id = current_world().add_prefab(asset);
            selection().set_selected(id);
            return id.is_valid();
        });
}

static void add_scene() {
    add_detached_widget<AssetSelector>(AssetType::Scene, "Add scene")->set_selected_callback(
        [](AssetId asset) {
            core::String name;
            if(const auto res = asset_store().name(asset)) {
                name = asset_store().filesystem()->filename(res.unwrap());
            }

            EditorWorld& world = current_world();
            world.add_scene(asset, world.create_collection_entity(name));
            return true;
        });
}


editor_action("Add debug entities", add_debug_entities)
editor_action("Add prefab", add_prefab)
editor_action("Add scene", add_scene)

static void populate_context_menu(EditorWorld& world, ecs::EntityId id = ecs::EntityId()) {
    if(EditorComponent* component = world.component<EditorComponent>(id)) {
        if(ImGui::Selectable("Rename")) {
            add_child_widget<Renamer>(component->name(), [=](std::string_view new_name) { component->set_name(new_name); return true; });
        }

        if(ImGui::Selectable(ICON_FA_TRASH " Delete")) {
            add_child_widget<DeletionDialog>(id);
        }

        ImGui::Separator();
    }

    if(ImGui::Selectable(ICON_FA_PLUS " New collection")) {
        world.set_parent(world.create_collection_entity("New collection"), id);
    }

    ecs::EntityId new_entity;
    if(ImGui::MenuItem(ICON_FA_PLUS " New empty entity")) {
        new_entity = world.create_entity();
    }

    ImGui::Separator();

    if(ImGui::MenuItem("Add prefab")) {
        add_prefab();
    }

    if(ImGui::MenuItem("Add scene")) {
        add_scene();
    }

    ImGui::Separator();

    if(ImGui::MenuItem(ICON_FA_LIGHTBULB " Add Point light")) {
        new_entity = world.create_named_entity("Point light", ecs::StaticArchetype<PointLightComponent>());
    }

    if(ImGui::MenuItem(ICON_FA_VIDEO " Add Spot light")) {
        new_entity = world.create_named_entity("Spot light", ecs::StaticArchetype<SpotLightComponent>());
    }

    if(new_entity.is_valid()) {
        world.set_parent(new_entity, id);
        selection().set_selected(new_entity);
    }
}

static void display_entity(ecs::EntityId id, EditorComponent* component, ecs::EntityId& hovered, ecs::EntityId& selected) {
    const bool display_hidden = app_settings().debug.display_hidden_entities;
    if(!component || (!display_hidden && component->is_hidden_in_editor())) {
        return;
    }

    EditorWorld& world = current_world();
    const bool is_selected = selected == id;
    const int flags = ImGuiTreeNodeFlags_SpanAvailWidth | (is_selected ? ImGuiTreeNodeFlags_Selected : 0);

    auto update_hover = [&] {
        if(ImGui::IsItemHovered()) {
            hovered = id;
        }
    };

    if(component->is_collection()) {
        const char* full_display_name = fmt_c_str(ICON_FA_BOX_OPEN " %##%", component->name(), id.as_u64());
        if(ImGui::TreeNodeEx(full_display_name, flags)) {
            update_hover();
            ImGui::Indent();
            for(ecs::EntityId id : component->children()) {
                display_entity(id, world.component<EditorComponent>(id), hovered, selected);
            }
            ImGui::Unindent();
            ImGui::TreePop();
        } else {
            update_hover();
        }
    } else {
        const std::string_view display_name = component->is_prefab() ? fmt("% (Prefab)", component->name()) : std::string_view(component->name());
        const char* full_display_name = fmt_c_str("% %##%", world.entity_icon(id), display_name, id.as_u64());
        if(ImGui::TreeNodeEx(full_display_name, flags | ImGuiTreeNodeFlags_Leaf)) {
            ImGui::TreePop();
        }
        update_hover();
        if(ImGui::IsItemClicked()) {
            selection().set_selected(id);
            selected = id;
        }
    }
}


EntityView::EntityView() : Widget(ICON_FA_CUBES " Entities") {
}

void EntityView::on_gui() {
    EditorWorld& world = current_world();

    if(ImGui::Button(ICON_FA_PLUS, math::Vec2(24))) {
        ImGui::OpenPopup("##entitymenu");
    }

    if(ImGui::BeginPopup("##entitymenu")) {
        populate_context_menu(world);
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    ImGui::Text("%u entities", u32(world.components<EditorComponent>().size()));

    ecs::EntityId hovered;
    if(ImGui::BeginChild("##entities", ImVec2(), true)) {
        imgui::alternating_rows_background();

        EditorWorld& world = current_world();
        for(auto&& [id, comp] : world.component_set<EditorComponent>()) {
            if(!comp.has_parent()/* || !world.exists(comp.parent())*/) {
                display_entity(id, &comp, hovered, _hovered);
            }
        }

        if(imgui::should_open_context_menu()) {
            ImGui::OpenPopup("##contextmenu");
            _hovered = hovered;
        }

        if(ImGui::BeginPopup("##contextmenu")) {
            populate_context_menu(world, _hovered);
            ImGui::EndPopup();
        }
    }
    ImGui::EndChild();
}

}

