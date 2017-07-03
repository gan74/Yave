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
#ifndef YAVE_MESHES_MESHDATA_H
#define YAVE_MESHES_MESHDATA_H

#include <yave/yave.h>
#include <y/io/Ref.h>

#include "Skeleton.h"

namespace yave {

class MeshData {

	public:
		static MeshData from_parts(core::Vector<Vertex>&& vertices, core::Vector<IndexedTriangle>&& triangles, core::Vector<SkinWeights>&& skin = {}, core::Vector<Bone>&& bones = {});
		static MeshData from_file(io::ReaderRef reader);
		void to_file(io::WriterRef writer) const;

		float radius() const;

		const core::Vector<Vertex>& vertices() const;
		const core::Vector<IndexedTriangle>& triangles() const;

		core::Vector<SkinnedVertex> skinned_vertices() const;
		const core::Vector<Bone>& bones() const;

	private:
		struct SkeletonData {
			core::Vector<SkinWeights> skin;
			core::Vector<Bone> bones;
		};

		float _radius = 0.0f;

		core::Vector<Vertex> _vertices;
		core::Vector<IndexedTriangle> _triangles;

		core::Unique<SkeletonData> _skeleton;



};

}

#endif // YAVE_MESHES_MESHDATA_H
