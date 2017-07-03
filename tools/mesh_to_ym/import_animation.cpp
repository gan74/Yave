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

#include "import.h"

#include <map>

namespace import {

AnimationChannel import_channel(aiNodeAnim* anim) {
	static constexpr usize None = 0x0;
	static constexpr usize Position = 0x1;
	static constexpr usize Scale = 0x2;
	static constexpr usize Rotation = 0x4;

	struct BoneTransformInfo {
		BoneTransform transform;
		usize info = None;
	};

	std::map<float, BoneTransformInfo> infos;
	for(usize i = 0; i != anim->mNumPositionKeys; ++i) {
		aiVectorKey key = anim->mPositionKeys[i];
		auto& info = infos[key.mTime];

		info.transform.position = {key.mValue.x, key.mValue.y, key.mValue.z};
		info.info |= Position;
	}
	for(usize i = 0; i != anim->mNumScalingKeys; ++i) {
		aiVectorKey key = anim->mScalingKeys[i];
		auto& info = infos[key.mTime];

		info.transform.scale = {key.mValue.x, key.mValue.y, key.mValue.z};
		info.info |= Scale;
	}
	for(usize i = 0; i != anim->mNumRotationKeys; ++i) {
		aiQuatKey key = anim->mRotationKeys[i];
		auto& info = infos[key.mTime];

		info.transform.rotation = {key.mValue.x, key.mValue.y, key.mValue.z, key.mValue.w};
		info.info |= Rotation;
	}

	if(infos.empty()) {
		fatal("Empty channel.");
	}


	if(infos.begin()->first > 0.0f) {
		auto first = infos.begin()->second;
		infos[0.0f] = first;
	}

	for(auto it = std::next(infos.begin()); it != infos.end(); ++it) {
		auto& info = it->second;
		const auto& prev = std::prev(it)->second.transform;

		if(!(info.info & Position)) {
			info.transform.position = prev.position;
		}
		if(!(info.info & Scale)) {
			info.transform.scale = prev.scale;
		}
		if(!(info.info & Rotation)) {
			info.transform.rotation = prev.rotation;
		}
	}


	core::Vector<AnimationChannel::BoneKey> keys;
	std::transform(infos.begin(), infos.end(), std::back_inserter(keys), [](const auto& k) { return AnimationChannel::BoneKey{k.first, k.second.transform}; });

	return AnimationChannel(anim->mNodeName.C_Str(), std::move(keys));
}

Animation import_animation(aiAnimation* anim) {
	if(!anim || anim->mDuration <= 0.0f || !anim->mNumChannels) {
		return fatal("Empty animation.");
	}

	auto channels = vector_with_capacity<AnimationChannel>(anim->mNumChannels);
	for(usize i = 0; i != anim->mNumChannels; ++i) {
		channels << import_channel(anim->mChannels[i]);
	}

	return Animation(anim->mDuration, std::move(channels));
}

}
