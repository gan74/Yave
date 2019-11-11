/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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
#ifndef EDITOR_CONTEXT_SETTINGS_H
#define EDITOR_CONTEXT_SETTINGS_H

#include <editor/editor.h>
#include <yave/window/EventHandler.h>
#include <yave/utils/serde.h>

namespace editor {

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

	y_serde3(z_near, fov, move_forward, move_backward, move_right, move_left, fps_sensitivity, trackball_sensitivity, dolly_sensitivity)

};

struct UiSettings {
	Key change_gizmo_mode = Key::R;
	Key change_gizmo_space = Key::Q;

	y_serde3(change_gizmo_mode, change_gizmo_space)
};

static_assert(std::is_standard_layout_v<CameraSettings> && std::is_trivially_copyable_v<CameraSettings>);


class Settings {
	public:
		Settings(bool load = true);
		~Settings();

		CameraSettings& camera();
		UiSettings& ui();

		y_serde2(_camera, _ui)
		y_serde3(_camera, _ui)

	private:
		CameraSettings _camera;
		UiSettings _ui;
};

}

#endif // EDITOR_CONTEXT_SETTINGS_H
