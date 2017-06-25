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

#include <y/core/Chrono.h>

namespace yave {

MeshData MeshData::from_file(io::ReaderRef reader) {
	const char* err_msg = "Unable to load mesh.";

	struct Header {
		u32 magic;
		u32 type;
		u32 version;

		bool is_valid() const {
			return magic == 0x65766179 &&
				   type == 1 &&
				   version == 3;
		}
	};


	core::DebugTimer _("MeshData::from_file()");

	auto decoder = io::Decoder(io::BuffReader(reader));

	Header header = decoder.decode<Header>().expected(err_msg);
	if(!header.is_valid()) {
		fatal(err_msg);
	}

	MeshData mesh;
	mesh.radius = decoder.decode<decltype(mesh.radius)>().expected(err_msg);
	mesh.vertices = decoder.decode<decltype(mesh.vertices)>().expected(err_msg);
	mesh.triangles = decoder.decode<decltype(mesh.triangles)>().expected(err_msg);

	return mesh;
}

}
