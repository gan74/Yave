/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

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

#include "MeshData.h"
#include <y/io/Decoder.h>
#include <y/io/BuffReader.h>

namespace yave {

MeshData MeshData::from_file(io::ReaderRef reader) {
	Y_TODO(this can be 10x faster)

	auto decoder = io::Decoder(io::BuffReader(reader));

	u32 magic = 0;
	u64 version = 0;
	MeshData mesh;

	decoder.decode<io::Byteorder::BigEndian>(magic);
	decoder.decode<io::Byteorder::BigEndian>(version);

	if(magic != 0x79617665 || version != 1) {
		return mesh;
	}

	using VertexAsArray = std::array<float, sizeof(Vertex) / sizeof(float)>;
	decoder.decode<io::Byteorder::BigEndian>(reinterpret_cast<core::Vector<VertexAsArray>&>(mesh.vertices));
	decoder.decode<io::Byteorder::BigEndian>(mesh.triangles);

	return mesh;
}

}
