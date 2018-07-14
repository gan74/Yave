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

#include "Settings.h"

#include <y/io/File.h>

namespace editor {

Settings::Settings() {
	if(auto file = io::File::open("settings.dat"); file.is_ok()) {
		try {
			deserialize(file.unwrap());
		} catch(std::exception& e) {
			log_msg("Unable to load settings: "_s + e.what(), Log::Error);
		}
	} else {
		log_msg("Unable to open settings file.", Log::Error);
	}
}

Settings::~Settings() {
	if(auto file = io::File::create("settings.dat"); file.is_ok()) {
		try {
			serialize(file.unwrap());
		} catch(std::exception& e) {
			log_msg("Unable to save settings: "_s + e.what(), Log::Error);
		}
	} else {
		log_msg("Unable to create settings file.", Log::Error);
	}
}

CameraSettings& Settings::camera() {
	return _camera;
}

}
