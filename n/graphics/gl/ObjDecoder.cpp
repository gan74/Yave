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

#include <n/defines.h>
#ifndef N_NO_GL

#include <iostream>
#include <n/io/File.h>
#include "MeshLoader.h"

namespace n {
namespace graphics {
namespace gl {

MeshLoader *MeshLoader::loader = 0;

class ObjDecoder : public MeshLoader::MeshDecoder<ObjDecoder, core::String>
{
	public:
		ObjDecoder() : MeshLoader::MeshDecoder<ObjDecoder, core::String>() {
		}

		internal::MeshInstance<> *operator()(core::String name) override {
			if(!name.endWith(".obj")) {
				return 0;
			}
			io::File file(name);
			return load(file);
		}

	private:
		internal::MeshInstance<> *load(io::File &file) {
		if(!file.open(io::IODevice::Read)) {
			std::cout<<file.getName()<<" not found"<<std::endl;
			return 0;
		}
		char *data = new char[file.size()];
		file.readBytes(data);
		core::Array<core::String> lines = core::String(data).split("\n");
		delete[] data;
		file.close();

		core::Array<math::Vec3> positions;
		positions.append(math::Vec3());
		core::Array<math::Vec3> normals;
		normals.append(math::Vec3());
		core::Array<math::Vec2> coords;
		coords.append(math::Vec2());

		for(const core::String &l : lines) {
			if(l.size() < 4) {
				continue;
			}
			if(l.beginWith("v ")) {
				core::Array<float> fl = l.subString(2).split(" ");
				if(fl.size() != 3) {
					return 0;
				}
				positions.append(math::Vec3(fl[0], fl[1], fl[2]));
			} else if(l.beginWith("vn ")) {
				core::Array<float> fl = l.subString(3).split(" ");
				if(fl.size() != 3) {
					return 0;
				}
				normals.append(math::Vec3(fl[0], fl[1], fl[2]));
			} else if(l.beginWith("vt ")) {
				core::Array<float> fl = l.subString(3).split(" ");
				if(fl.size() != 2) {
					return 0;
				}
				coords.append(math::Vec2(fl[0], fl[1]));
			}
		}
		//std::cout<<positions.size()<<" "<<normals.size()<<" "<<coords.size()<<std::endl;
		core::Map<math::Vec3ui, uint> vmap;
		bool smooth = false;
		TriangleBuffer<> tr;
		for(const core::String &l : lines) {
			if(l.beginWith("f ")) {
				core::Array<core::String> fl = l.subString(2).split(" ");
				uint face[] = {0, 0, 0};
				if(fl.size() != 3) {
					return 0;
				}
				for(uint i = 0; i != 3; i++) {
					core::Array<uint> uis = fl[i].split("/", true).mapped([](const core::String &str) { return str.isEmpty() ? 0 : uint(str); });
					for(; uis.size() < 3;) {
						uis += 0;
					}
					math::Vec3ui v(uis[0], uis[1], smooth ? 0 : uis[2]);
					core::Map<math::Vec3ui, uint>::const_iterator it = vmap.find(v);
					if(it == vmap.end()) {
						uint vy = v.y();
						if(!vy) {
							math::Vec3 p = positions[v.x()].normalized();
							vy = coords.size();
							coords.append(math::Vec2(acos(p.x()) / math::pi<>(), asin(p.z()) / math::pi<float>()));
						}
						if(v.x() >= positions.size() || v.z() >= normals.size() || vy >= coords.size()) {
							return 0;
						}
						face[i] = vmap[v] = tr.append(Vertex<>(positions[v.x()], normals[v.z()], coords[vy]));
					} else {
						face[i] = (*it)._2;
					}
				}
				tr.append(face[0], face[1], face[2]);
			} else if(l.beginWith("s ")) {
				core::String sm = l.subString(2).toLower().filtered([](char c) { return !isspace(c); });
				smooth = !(sm == "off" || sm == "0");
			} else if(l.beginWith("usemtl")) {
				if(!tr.getTriangles().isEmpty()) {
					fatal("Unsupported feature");
				}
			}
		}
		return new internal::MeshInstance<>(std::move(tr.freezed()));
	}
};

}
}
}

#endif

