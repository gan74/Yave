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
#ifndef YAVE_MESHES_SKELETON_H
#define YAVE_MESHES_SKELETON_H

#include "Vertex.h"
#include "Bone.h"

namespace yave {

class Skeleton {
	public:
		static constexpr usize max_bones = 256;

		Skeleton() = default;

		Skeleton(const core::Vector<Bone>& bones);

		const core::Vector<Bone>& bones() const;
		const core::Vector<math::Transform<>>& bone_transforms() const;
		const core::Vector<math::Transform<>>& inverse_absolute_transforms() const;

	private:
		core::Vector<Bone> _bones;
		core::Vector<math::Transform<>> _transforms;
		core::Vector<math::Transform<>> _inverses;
};

}

#endif // YAVE_MESHES_SKELETON_H
