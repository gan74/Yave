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

#include "SceneCell.h"

namespace yave {

template<typename T>
static AABB compute_AABB(const core::Vector<T>& objs) {
	if(objs.is_empty()) {
		return AABB{};
	}

	math::Vec3 min(std::numeric_limits<float>::max());
	math::Vec3 max(std::numeric_limits<float>::min());

	for(const auto& t : objs) {
		const auto& pos = t.position();
		auto r = t.radius();
		for(usize i = 0; i != 3; ++i) {
			min[i] = std::min(min[i], pos[i] - r);
			max[i] = std::max(max[i], pos[i] + r);
		}
	}
	return AABB{min, max};
}


SceneCell::SceneCell(core::Vector<StaticMeshInstance>&& meshes) : _statics(std::move(meshes)), _aabb(compute_AABB(_statics)) {
}

const AABB& SceneCell::aabb() const {
	return _aabb;
}

const core::Vector<StaticMeshInstance>& SceneCell::static_meshes() const {
	return _statics;
}

}
