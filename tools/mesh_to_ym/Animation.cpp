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

#include "Animation.h"

#include <unordered_map>

#include <iostream>


const String& Animation::name() const {
	return _name;
}



AnimationChannel AnimationChannel::from_assimp(aiNodeAnim* anim) {
	std::unordered_map<float, BonePose> keys;

	static_assert(sizeof(aiVector3D) == sizeof(math::Vec3), "aiVector3 should be 3 floats");
	static_assert(sizeof(aiQuaternion) == sizeof(math::Vec4), "aiQuaterniont should be 4 floats");

	for(usize i = 0; i != anim->mNumPositionKeys; ++i) {
		aiVectorKey key = anim->mPositionKeys[i];
		keys[key.mTime].position = reinterpret_cast<const math::Vec3&>(key.mValue);
	}

	for(usize i = 0; i != anim->mNumScalingKeys; ++i) {
		aiVectorKey key = anim->mScalingKeys[i];
		keys[key.mTime].scaling = reinterpret_cast<const math::Vec3&>(key.mValue);
	}

	for(usize i = 0; i != anim->mNumRotationKeys; ++i) {
		aiQuatKey key = anim->mRotationKeys[i];
		keys[key.mTime].quaternion = reinterpret_cast<const math::Vec4&>(key.mValue);
	}

	if(anim->mNumPositionKeys != keys.size()) {
		log_msg("Invalid positions.", Log::Error);
	}

	if(anim->mNumRotationKeys != keys.size()) {
		log_msg("Invalid rotations.", Log::Error);
	}

	if(anim->mNumScalingKeys != keys.size()) {
		log_msg("Invalid scales.", Log::Error);
	}

	log_msg("channel for " + str(anim->mNodeName.C_Str()) + " has " +  str(keys.size()) + " keys");


	AnimationChannel channel;
	channel._name = anim->mNodeName.C_Str();
	std::transform(keys.begin(), keys.end(), std::back_inserter(channel._keys), [](const auto& k) { return Key{k.first, k.second}; });
	sort(channel._keys.begin(), channel._keys.end(), [](const auto& a, const auto& b) { return a.time < b.time; });

	return channel;
}

io::Writer::Result AnimationChannel::write(io::WriterRef writer) const {
	writer->write_one(u32(_name.size()));
	writer->write(_name.data(), _name.size());

	writer->write_one(u32(_keys.size()));
	writer->write(_keys.begin(), _keys.size() * sizeof(Key));

	return Ok();
}

Result<Animation> Animation::from_assimp(aiAnimation* anim) {
	if(!anim || anim->mDuration <= 0.0f || !anim->mNumChannels) {
		return Err();
	}

	Animation an;
	an._duration = anim->mDuration;

	log_msg("duration: " + str(an._duration));

	for(usize i = 0; i != anim->mNumChannels; ++i) {
		an._channels << AnimationChannel::from_assimp(anim->mChannels[i]);
	}

	return Ok(std::move(an));
}

io::Writer::Result Animation::write(io::WriterRef writer) const {
	u32 magic = 0x65766179;
	u32 type = 3;
	u32 version = 1;

	writer->write_one(magic);
	writer->write_one(type);
	writer->write_one(version);

	writer->write_one(u32(_channels.size()));
	writer->write_one(_duration);

	for(const auto& channel : _channels) {
		channel.write(writer);
	}

	return Ok();
}

