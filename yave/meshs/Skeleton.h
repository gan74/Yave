/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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
#ifndef YAVE_MESHS_SKELETON_H
#define YAVE_MESHS_SKELETON_H

#include "Vertex.h"
#include <y/math/Transform.h>

namespace yave {

struct Bone {
	core::String name;
	u32 parent;
	math::Transform<> transform;
};

class Skeleton {
	public:
		static constexpr usize max_bones = 256;

		Skeleton() = default;

		Skeleton(const core::Vector<Bone>& bones) :
				_bones(bones),
				_root(std::find_if(_bones.begin(), _bones.end(), [](const auto& b) { return b.parent == u32(-1); })) {

			if(!_root || _root == _bones.end()) {
				fatal("Skeleton is not rooted.");
			}

			if(_bones.size() > max_bones) {
				fatal("Bone count exceeds max_bones.");
			}
		}

		const core::Vector<Bone>& bones() const {
			return _bones;
		}

	private:
		core::Vector<Bone> _bones;
		Bone* _root = nullptr;
};

}

#endif // YAVE_MESHS_SKELETON_H
