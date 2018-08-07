/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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
#ifndef EDITOR_CONTEXT_ICONS_H
#define EDITOR_CONTEXT_ICONS_H

#include <editor/editor.h>

#include <yave/graphics/images/ImageData.h>
#include <yave/graphics/images/Image.h>
#include <yave/graphics/images/ImageView.h>

namespace editor {
namespace images {
ImageData light();
ImageData save();
ImageData load();
}

class Icons : public DeviceLinked {
	public:
		Icons(DevicePtr dptr) : DeviceLinked(dptr) {
			_light = Texture(device(), images::light());
			_light_view = _light;

			_save = Texture(device(), images::save());
			_save_view = _save;

			_load = Texture(device(), images::load());
			_load_view = _load;
		}

		TextureView& light() {
			return _light_view;
		}

		TextureView& save() {
			return _save_view;
		}

		TextureView& load() {
			return _load_view;
		}

	private:

		Texture _light;
		TextureView _light_view;

		Texture _save;
		TextureView _save_view;

		Texture _load;
		TextureView _load_view;
};

}

#endif // EDITOR_CONTEXT_ICONS_H
