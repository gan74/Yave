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

#include "AnimationChannel.h"

namespace yave {

AnimationChannel::AnimationChannel(const core::String& name, core::Vector<BoneKey>&& keys) : _name(name), _keys(keys) {
	if(_keys.is_empty()) {
		y_fatal("Empty animation channel.");
	}
}

math::Transform<> AnimationChannel::bone_transform(float time) const {
	auto key = std::find_if(_keys.begin(), _keys.end(), [=](const auto& k) { return k.time > time; });

	auto next = key == _keys.end() ? _keys.begin() : key;
	key = key == _keys.begin() ? key : std::prev(key);

	float delta = next->time - key->time;
	delta = delta < 0.0f ? delta + _keys.last().time : delta;

	float factor = (time - key->time) / delta;

	return key->local_transform.lerp(next->local_transform, factor);
}


const core::String& AnimationChannel::name() const {
	return _name;
}

core::ArrayView<AnimationChannel::BoneKey> AnimationChannel::keys() const {
	return _keys;
}

}
