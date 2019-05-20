/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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
#ifndef YAVE_ANIMATIONS_ANIMATIONCHANNEL_H
#define YAVE_ANIMATIONS_ANIMATIONCHANNEL_H

#include <yave/meshes/Bone.h>

namespace yave {

class AnimationChannel {
	public:
		struct BoneKey {
			float time;
			BoneTransform local_transform;
		};

		y_serialize2(_name, _keys)
		y_deserialize2(serde2::func([](const core::String& name, core::Vector<BoneKey>&& keys) { return AnimationChannel(name, std::move(keys)); }))


		AnimationChannel() = default;
		AnimationChannel(const core::String& name, core::Vector<BoneKey>&& keys);

		math::Transform<> bone_transform(float time) const;


		const core::String& name() const;
		core::ArrayView<BoneKey> keys() const;

	private:
		core::String _name;
		core::Vector<BoneKey> _keys;
};

}

#endif // YAVE_ANIMATIONS_ANIMATIONCHANNEL_H
