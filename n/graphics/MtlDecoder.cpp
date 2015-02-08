/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/

#ifndef N_GRAPHICS_MTLDECODER
#define N_GRAPHICS_MTLDECODER


#include <iostream>
#include <n/io/File.h>
#include "MaterialLoader.h"

namespace n {
namespace graphics {

MaterialLoader *MaterialLoader::loader = 0;

class MtlDecoder : public MaterialLoader::MaterialDecoder<MtlDecoder, core::String, core::String>
{
	public:
		MtlDecoder() : MaterialLoader::MaterialDecoder<MtlDecoder, core::String, core::String>() {
		}

		internal::Material<> *operator()(core::String fileName, core::String name) override {
			if(!fileName.endWith(".mtl")) {
				return 0;
			}
			io::File file(fileName);
			return load(file, name);
		}

	private:
		internal::Material<> *load(io::File &file, const core::String &name) {
		if(!file.open(io::IODevice::Read)) {
			std::cerr<<file.getName()<<" not found"<<std::endl;
			return 0;
		}
		char *data = new char[file.size()];
		file.readBytes(data);
		core::Array<core::String> lines = core::String(data).split("\n");
		delete[] data;
		file.close();

		internal::Material<> *mat = 0;
		for(const core::String &l : lines) {
			if(l.beginWith("newmtl ")) {
				if(l.subString(7).filtered([](char c) { return !isspace(c); }) == name) {
					if(mat) {
						fatal("Material already defined");
					}
					mat = new internal::Material<>();
				} else if(mat) {
					break;
				}
			} else if(mat) {
				if(l.toLower().beginWith("kd ")) {
					core::Array<float> fl = l.subString(3).split(" ");
					if(fl.size() != 3) {
						std::cerr<<"Invalid color"<<std::endl;
						return 0;
					}
					mat->color = Color<>(fl[0], fl[1], fl[2], 1);
				} else if(l.toLower().beginWith("ks ")) {
					core::Array<float> fl = l.subString(3).split(" ");
					if(fl.size() != 3) {
						std::cerr<<"Invalid color"<<std::endl;
						return 0;
					}
					mat->specular = (fl[0] + fl[1] + fl[2]) / 3;
				}
			}
		}
		return mat;
	}
};

}
}



#endif // N_GRAPHICS_MTLDECODER

