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
#ifndef EDITOR_SETTINGS_H
#define EDITOR_SETTINGS_H

#include <editor/editor.h>

#include <yave/window/EventHandler.h>

#include <y/reflect/reflect.h>
#include <y/core/String.h>

namespace editor {

struct EditorSettings {
    core::String world_file = "../world.yw3";
    core::String asset_store = "../store.sqlite3";

    y_reflect(world_file, asset_store)
};

struct CameraSettings {
    float z_near = 1.0f;
    float fov = 60.0f;

    // FPS
    Key move_forward = Key::W;
    Key move_backward = Key::S;
    Key move_right = Key::D;
    Key move_left = Key::A;
    float fps_sensitivity = 4.0f;

    // Houdini
    float trackball_sensitivity = 6.0f;
    float dolly_sensitivity = 2.5f;

    // Other camera
    Key center_on_obj = Key::H;

    y_reflect(z_near, fov,
              move_forward, move_backward, move_right, move_left,
              fps_sensitivity, trackball_sensitivity, dolly_sensitivity,
              center_on_obj)

};

struct UiSettings {
    Key change_gizmo_mode = Key::R;
    Key change_gizmo_space = Key::Q;


    y_reflect(change_gizmo_mode, change_gizmo_space)
};

struct PerfSettings {
};

struct DebugSettings {
    usize entity_count = 1000;
    bool display_octree = false;
    bool display_selected_bbox = false;
    bool display_hidden_entities = false;

    y_reflect(entity_count, display_octree, display_selected_bbox, display_hidden_entities)
};


class Settings {
    public:
        Settings(bool load = true);
        ~Settings();

        EditorSettings editor;
        CameraSettings camera;
        UiSettings ui;
        PerfSettings perf;
        DebugSettings debug;

        y_reflect(editor, camera, ui, perf, debug)
};

}

#endif // EDITOR_CONTEXT_SETTINGS_H

