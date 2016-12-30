/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
		log_msg("Unable to load mesh.", LogType::Error);
		return mesh;
	}

	using VertexAsArray = std::array<float, sizeof(Vertex) / sizeof(float)>;
	decoder.decode<io::Byteorder::BigEndian>(reinterpret_cast<core::Vector<VertexAsArray>&>(mesh.vertices));
	decoder.decode<io::Byteorder::BigEndian>(mesh.triangles);

	return mesh;
}

}
